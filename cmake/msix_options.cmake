# Copyright (C) 2019 Microsoft.  All rights reserved.
# See LICENSE file in the project root for full license information.
# Validates CMake options for the MSIX SDK.

option(WIN32 "Build for Win32"   OFF)
option(MACOS "Build for MacOS"   OFF)
option(IOS   "Build for iOS"     OFF)
option(AOSP  "Build for Android" OFF)
option(LINUX "Build for Linux"   OFF)

option(USE_VALIDATION_PARSER "Turn on to validates using the resouce schemas. Default (OFF) validates XML files are just valid XML" OFF)
option(USE_SHARED_ZLIB "Choose the type of dependency for zlib, Use the -DUSE_SHARED_ZLIB=on to have a shared dependency. Default is 'off' (static)" OFF)
option(USE_STATIC_MSVC "Windows only. Pass /MT as a compiler flag to use the staic version of the run-time library. Default is 'off' (dynamic)" OFF)
option(SKIP_BUNDLES "Removes bundle functionality from the MSIX SDK. Default is 'off'" OFF)
option(MSIX_PACK "Include packaging features for the MSIX SDK. Not supported for mobile. Default is 'off'" OFF)
option(USE_MSIX_SDK_ZLIB "Use zlib implementation under lib/zlib. If off, uses inbox compression library. For Windows and Linux this is no-opt." OFF)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel. Use the -DCMAKE_BUILD_TYPE=[option] to specify.")
set(XML_PARSER "" CACHE STRING "Choose the type of parser, options are: [xerces, msxml6, javaxml].  Use the -DXML_PARSER=[option] to specify.")
set(CRYPTO_LIB "" CACHE STRING "Choose the cryptography library to use, options are: [openssl, crypt32].  Use the -DCRYPTO_LIB=[option] to specify.")

# Enforce that target platform is specified.
if((NOT WIN32) AND (NOT MACOS) AND (NOT IOS) AND (NOT AOSP) AND (NOT LINUX))
    message(FATAL_ERROR "You must specify one of: -D[WIN32|MACOS|IOS|AOSP|LINUX]=on")
endif()

if(USE_STATIC_MSVC)
    if(NOT WIN32)
        message(FATAL_ERROR "-DUSE_STATIC_MSVC=on can only be used for Windows")
    endif()
    # By default these flags have /MD set. Modified it to use /MT instead.
    foreach(buildType RELEASE MINSIZEREL RELWITHDEBINFO)
        set(cxxFlag "CMAKE_CXX_FLAGS_${buildType}")
        string(REPLACE "/MD" "/MT" ${cxxFlag} "${${cxxFlag}}")
    endforeach()
    set(cxxFlagDebug "CMAKE_CXX_FLAGS_DEBUG")
    string(REPLACE "/MDd" "/MTd" ${cxxFlagDebug} "${${cxxFlagDebug}}")
endif()

# Set xml parser if not set
if(NOT XML_PARSER)
    if(WIN32)
        set(XML_PARSER msxml6 CACHE STRING "XML Parser not defined. Using msxml6" FORCE)
    elseif(AOSP)
        set(XML_PARSER javaxml CACHE STRING "XML Parser not defined. Using javaxml" FORCE)
    elseif(MAC)
        if(MSIX_PACK)
            set(XML_PARSER xerces CACHE STRING "XML Parser not defined. Using xerces" FORCE)
        else()
            set(XML_PARSER applexml CACHE STRING "XML Parser not defined. Using applexml" FORCE)
        endif()
    elseif(IOS)
        set(XML_PARSER applexml CACHE STRING "XML Parser not defined. Using applexml" FORCE)
    else()
        set(XML_PARSER xerces CACHE STRING "XML Parser not defined. Using xerces" FORCE)
    endif()
endif()

# Set crypto library if not set
if(NOT CRYPTO_LIB)
    if(WIN32)
        set(CRYPTO_LIB crypt32 CACHE STRING "Crypto Lib not defined. Using crypt32" FORCE)
    else()
        set(CRYPTO_LIB openssl CACHE STRING "Crypto Lib not defined. Using openssl" FORCE)
    endif()
endif()

# Validates PACK options are correct
if(MSIX_PACK)
    if(AOSP OR IOS)
        message(FATAL_ERROR "Packaging is not supported for mobile devices.")
    elseif(MAC)
        if(NOT USE_MSIX_SDK_ZLIB)
            message(FATAL_ERROR "Using libCompression APIs and packaging features is not supported. Use -DUSE_MSIX_SDK_ZLIB=on")
        endif()
        if(NOT (XML_PARSER MATCHES xerces))
            message(FATAL_ERROR "Xerces is the only supported parser for MacOS pack. Use -DXML_PARSER=xerces")
        endif()
    endif()
    if(NOT USE_VALIDATION_PARSER)
        message(FATAL_ERROR "Packaging requires validation parser. Use -DUSE_VALIDATION_PARSER=on")
    endif()
endif()

# Compression
set(COMPRESSION_LIB "zlib")
if(((IOS) OR (MACOS)) AND (NOT USE_MSIX_SDK_ZLIB))
    set(COMPRESSION_LIB "libCompression")
elseif((AOSP) AND (NOT USE_MSIX_SDK_ZLIB))
    set(COMPRESSION_LIB "inbox zlib")
endif()

message(STATUS "MSIX SDK options validation")
message(STATUS "Configuration:")
if(WIN32)
    message(STATUS "\tPlatform            = WIN32")
elseif(MACOS)
    message(STATUS "\tPlatform            = MacOS")
elseif(IOS)
    message(STATUS "\tPlatform = iOS")
    message(STATUS "\tPlatform            = iOS")
elseif(AOSP)
    message(STATUS "\tPlatform            = Android")
else()
    message(STATUS "\tPlatform            = Linux")
endif()

message(STATUS "\tPackaging support   = ${MSIX_PACK}")
if(SKIP_BUNDLES)
    message(STATUS "\tBundle support      = off")
else()
    message(STATUS "\tBundle support      = on")
endif()

message(STATUS "\tCompression library = ${COMPRESSION_LIB}")
message(STATUS "\tXML Parser          = ${XML_PARSER} with validation parser ${USE_VALIDATION_PARSER}")
message(STATUS "\tCrypto library      = ${CRYPTO_LIB}")