#pragma once
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <iterator>

#include "StreamBase.hpp"
#include "StorageObject.hpp"
#include "VerifierObject.hpp"
#include "AppxPackaging.hpp"
#include "ComHelper.hpp"
#include "UnicodeConversion.hpp"
#include "AppxFactory.hpp"
#include "IXml.hpp"
#include "BlockMapStream.hpp"
#include "xercesc/util/XMLString.hpp"

namespace MSIX {

    class AppxBlockMapBlock : public MSIX::ComClass<AppxBlockMapBlock, IAppxBlockMapBlock>
    {
    public:
        AppxBlockMapBlock(IMSIXFactory* factory, Block* block) :
            m_factory(factory),
            m_block(block)
        {}

        // IAppxBlockMapBlock
        HRESULT STDMETHODCALLTYPE GetHash(UINT32* bufferSize, BYTE** buffer) override
        {
            return ResultOf([&]{
                ThrowHrIfFailed(m_factory->MarshalOutBytes(m_block->hash, bufferSize, buffer));
            });
        }

        HRESULT STDMETHODCALLTYPE GetCompressedSize(UINT32* size) override
        {
            return ResultOf([&]{
                ThrowErrorIf(Error::InvalidParameter, (size == nullptr), "bad pointer");
                *size = static_cast<UINT32>(m_block->compressedSize);
            });
        }

    private:
        IMSIXFactory*   m_factory;
        Block*          m_block;
    };

    class AppxBlockMapBlocksEnumerator : public MSIX::ComClass<AppxBlockMapBlocksEnumerator, IAppxBlockMapBlocksEnumerator>
    {
    protected:
        std::vector<ComPtr<IAppxBlockMapBlock>>* m_blocks;
        std::size_t                              m_cursor = 0;

    public:
        AppxBlockMapBlocksEnumerator(std::vector<ComPtr<IAppxBlockMapBlock>>* blocks) :
            m_blocks(blocks)
        {}

        // IAppxBlockMapBlocksEnumerator
        HRESULT STDMETHODCALLTYPE GetCurrent(IAppxBlockMapBlock** block) override
        {
            return ResultOf([&]{
                ThrowErrorIf(Error::InvalidParameter, (block == nullptr || *block != nullptr), "bad pointer");
                *block = m_blocks->at(m_cursor).Get();
                (*block)->AddRef();
            });
        }

        HRESULT STDMETHODCALLTYPE GetHasCurrent(BOOL* hasCurrent) override
        {   return ResultOf([&]{
                ThrowErrorIfNot(Error::InvalidParameter, (hasCurrent), "bad pointer");
                *hasCurrent = (m_cursor != m_blocks->size()) ? TRUE : FALSE;
            });
        }

        HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasNext) override
        {   return ResultOf([&]{
                ThrowErrorIfNot(Error::InvalidParameter, (hasNext), "bad pointer");
                *hasNext = (++m_cursor != m_blocks->size()) ? TRUE : FALSE;
            });
        }
    };

    class AppxBlockMapFile : public MSIX::ComClass<AppxBlockMapFile, IAppxBlockMapFile>
    {
    public:
        AppxBlockMapFile(
            IMSIXFactory* factory,
            std::vector<Block>* blocks,
            std::uint32_t localFileHeaderSize,
            const std::string& name,
            std::uint64_t uncompressedSize
        ) :
            m_factory(factory),
            m_blocks(blocks),
            m_localFileHeaderSize(localFileHeaderSize),
            m_name(name),
            m_uncompressedSize(uncompressedSize)
        {
        }

        // IAppxBlockMapFile
        HRESULT STDMETHODCALLTYPE GetBlocks(IAppxBlockMapBlocksEnumerator **blocks) override
        {
            return ResultOf([&]{
                if (m_blockMapBlocks.empty())
                {   m_blockMapBlocks.reserve(m_blocks->size());
                    std::transform(
                        m_blocks->begin(),
                        m_blocks->end(),
                        std::back_inserter(m_blockMapBlocks),
                        [&](auto item){
                            return ComPtr<IAppxBlockMapBlock>::Make<AppxBlockMapBlock>(m_factory, &item);
                        }
                    );
                }
                ThrowErrorIf(Error::InvalidParameter, (blocks == nullptr || *blocks != nullptr), "bad pointer.");
                *blocks = ComPtr<IAppxBlockMapBlocksEnumerator>::Make<AppxBlockMapBlocksEnumerator>(&m_blockMapBlocks).Detach();
            });
        }

        HRESULT STDMETHODCALLTYPE GetLocalFileHeaderSize(UINT32* lfhSize) override
        {   // Retrieves the size of the zip local file header of the associated zip file item
            return ResultOf([&]{
                ThrowErrorIf(Error::InvalidParameter, (lfhSize == nullptr), "bad pointer");
                *lfhSize = static_cast<UINT32>(m_localFileHeaderSize);
            });
        }

        HRESULT STDMETHODCALLTYPE GetName(LPWSTR *name) override
        {
            return ResultOf([&]{
                ThrowHrIfFailed(m_factory->MarshalOutString(m_name, name));
            });
        }

        HRESULT STDMETHODCALLTYPE GetUncompressedSize(UINT64 *size) override
        {
            return ResultOf([&]{
                ThrowErrorIf(Error::InvalidParameter, (size == nullptr), "bad pointer");
                *size = static_cast<UINT64>(m_uncompressedSize);
            });
        }

        HRESULT STDMETHODCALLTYPE ValidateFileHash(IStream *fileStream, BOOL *isValid) override
        {
            return ResultOf([&]{
                // TODO: Implement...
                throw Exception(Error::NotImplemented);
            });
        }

    private:

        std::vector<ComPtr<IAppxBlockMapBlock>> m_blockMapBlocks;
        std::vector<Block>* m_blocks;
        IMSIXFactory*       m_factory;
        std::uint32_t       m_localFileHeaderSize;
        std::string         m_name;
        std::uint64_t       m_uncompressedSize;
    };

    class AppxBlockMapFilesEnumerator : public MSIX::ComClass<AppxBlockMapFilesEnumerator, IAppxBlockMapFilesEnumerator>
    {
    protected:
        ComPtr<IAppxBlockMapReader> m_reader;
        std::vector<std::string>    m_files;
        std::size_t                 m_cursor = 0;

    public:
        AppxBlockMapFilesEnumerator(IAppxBlockMapReader* reader, std::vector<std::string>&& files) :
            m_reader(reader),
            m_files(files)
        {}

        // IAppxBlockMapFilesEnumerator
        HRESULT STDMETHODCALLTYPE GetCurrent(IAppxBlockMapFile** block) override
        {
            return ResultOf([&]{
                ThrowErrorIf(Error::Unexpected, (m_cursor >= m_files.size()), "index out of range");
                ThrowHrIfFailed(m_reader->GetFile(utf8_to_utf16(m_files.at(m_cursor)).c_str(), block));
            });
        }

        HRESULT STDMETHODCALLTYPE GetHasCurrent(BOOL* hasCurrent) override
        {   return ResultOf([&]{
                ThrowErrorIfNot(Error::InvalidParameter, (hasCurrent), "bad pointer");
                *hasCurrent = (m_cursor != m_files.size()) ? TRUE : FALSE;
            });
        }

        HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasNext) override
        {   return ResultOf([&]{
                ThrowErrorIfNot(Error::InvalidParameter, (hasNext), "bad pointer");
                *hasNext = (++m_cursor != m_files.size()) ? TRUE : FALSE;
            });
        }
    };

    // Object backed by AppxBlockMap.xml
    class AppxBlockMapObject : public MSIX::ComClass<AppxBlockMapObject, IAppxBlockMapReader, IVerifierObject, IStorageObject>
    {
    public:
        AppxBlockMapObject(IMSIXFactory* factory, ComPtr<IStream>& stream);

        // IVerifierObject
        const std::string& GetPublisher() override { throw Exception(Error::NotSupported); }
        bool HasStream() override { return m_stream.Get() != nullptr; }
        MSIX::ComPtr<IStream> GetStream() override { return m_stream; }
        MSIX::ComPtr<IStream> GetValidationStream(const std::string& part, IStream* stream) override;

        // IAppxBlockMapReader
        HRESULT STDMETHODCALLTYPE GetFile(LPCWSTR filename, IAppxBlockMapFile **file) override;
        HRESULT STDMETHODCALLTYPE GetFiles(IAppxBlockMapFilesEnumerator **enumerator) override;
        HRESULT STDMETHODCALLTYPE GetHashMethod(IUri **hashMethod) override;
        HRESULT STDMETHODCALLTYPE GetStream(IStream **blockMapStream) override;

        // IStorageObject methods
        std::string               GetPathSeparator() override;
        std::vector<std::string>  GetFileNames(FileNameOptions options) override;
        std::pair<bool,IStream*>  GetFile(const std::string& fileName) override;
        void                      RemoveFile(const std::string& fileName) override;
        IStream*                  OpenFile(const std::string& fileName, MSIX::FileStream::Mode mode) override;
        void                      CommitChanges() override;

    protected:
        std::map<std::string, std::vector<Block>>        m_blockMap;
        std::map<std::string, ComPtr<IAppxBlockMapFile>> m_blockMapfiles;
        IMSIXFactory*   m_factory;
        ComPtr<IStream> m_stream;
    };
}