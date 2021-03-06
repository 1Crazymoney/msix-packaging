#include <windows.h>

#include <shlobj_core.h>
#include <CommCtrl.h>

#include "WriteDevirtualizedRegistry.hpp"
#include "GeneralUtil.hpp"
#include <TraceLoggingProvider.h>
#include "MsixTraceLoggingProvider.hpp"
#include "Constants.hpp"
#include "RegistryDevirtualizer.hpp"

using namespace MsixCoreLib;

const PCWSTR WriteDevirtualizedRegistry::HandlerName = L"WriteDevirtualizedRegistry";

HRESULT WriteDevirtualizedRegistry::ExecuteForAddRequest()
{
    if (m_msixRequest->GetRegistryDevirtualizer() != nullptr)
    {
        const HRESULT hrWriteRegistry = m_msixRequest->GetRegistryDevirtualizer()->Run(false);
        if (FAILED(hrWriteRegistry))
        {
            const HRESULT hrUnloadMountedHive = m_msixRequest->GetRegistryDevirtualizer()->UnloadMountedHive();
            TraceLoggingWrite(g_MsixTraceLoggingProvider,
                "Unable to write registry",
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingValue(hrWriteRegistry, "HR"),
                TraceLoggingValue(hrUnloadMountedHive, "HR"));

            return hrWriteRegistry;
        }
        RETURN_IF_FAILED(m_msixRequest->GetRegistryDevirtualizer()->UnloadMountedHive());
    }
    return S_OK;
}

HRESULT WriteDevirtualizedRegistry::ExecuteForRemoveRequest()
{
    if (m_msixRequest->GetRegistryDevirtualizer() != nullptr)
    {
        const HRESULT hrRemoveRegistry = m_msixRequest->GetRegistryDevirtualizer()->Run(true);
        if (FAILED(hrRemoveRegistry))
        {
            TraceLoggingWrite(g_MsixTraceLoggingProvider,
                "Unable to remove registry",
                TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
                TraceLoggingValue(hrRemoveRegistry, "HR"));
        }
        RETURN_IF_FAILED(m_msixRequest->GetRegistryDevirtualizer()->UnloadMountedHive());
    }
    return S_OK;
}

HRESULT WriteDevirtualizedRegistry::CreateHandler(MsixRequest * msixRequest, IPackageHandler ** instance)
{
    std::unique_ptr<WriteDevirtualizedRegistry > localInstance(new WriteDevirtualizedRegistry(msixRequest));
    if (localInstance == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    *instance = localInstance.release();

    return S_OK;
}