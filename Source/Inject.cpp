/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Inject.cpp
 *   Utility implementations for interfacing with injection code.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Inject.h"
#include "InjectResult.h"

#include <cstddef>
#include <cstring>


namespace Hookshot
{
    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "Inject.h" for documentation.

    InjectInfo::InjectInfo(void) : injectTrampolineStart(NULL), injectTrampolineAddressMarker(NULL), injectTrampolineEnd(NULL), injectCodeStart(NULL), injectCodeBegin(NULL), injectCodeEnd(NULL), injectFileHandle(INVALID_HANDLE_VALUE), injectFileMappingHandle(INVALID_HANDLE_VALUE), injectFileBase(NULL), initializationResult(EInjectResult::InjectResultFailure)
    {
        // Figure out the name of the module that is to be loaded.
        TCHAR moduleFilename[Globals::kPathBufferLength];
        
        if (false == FillInjectCodeFilename(moduleFilename, _countof(moduleFilename)))
        {
            initializationResult = EInjectResult::InjectResultErrorCannotGenerateInjectCodeFilename;
            return;
        }

        // Attempt to open the module as a normal file.
        injectFileHandle = CreateFile(moduleFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if ((NULL == injectFileHandle) || (INVALID_HANDLE_VALUE == injectFileHandle))
        {
            injectFileMappingHandle = INVALID_HANDLE_VALUE;
            initializationResult = EInjectResult::InjectResultErrorCannotLoadInjectCode;
            return;
        }

        // Attempt to create a file mapping for the just-opened module file.
        injectFileMappingHandle = CreateFileMapping(injectFileHandle, NULL, PAGE_READONLY | SEC_IMAGE_NO_EXECUTE, 0, 0, NULL);
        
        if ((NULL == injectFileMappingHandle) || (INVALID_HANDLE_VALUE == injectFileMappingHandle))
        {
            injectFileMappingHandle = INVALID_HANDLE_VALUE;
            initializationResult = EInjectResult::InjectResultErrorCannotLoadInjectCode;
            return;
        }

        // Attempt to map the module file into memory.
        injectFileBase = MapViewOfFile(injectFileMappingHandle, FILE_MAP_READ, 0, 0, 0);

        //
        // TODO - Parse the mapped file and fill pointer values.
        //
    }

    // --------

    InjectInfo::~InjectInfo(void)
    {
        if (NULL != injectFileBase)
            UnmapViewOfFile(injectFileBase);

        if (INVALID_HANDLE_VALUE != injectFileMappingHandle)
            CloseHandle(injectFileMappingHandle);

        if (INVALID_HANDLE_VALUE != injectFileHandle)
            CloseHandle(injectFileHandle);
    }


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Inject.h" for documentation.

    bool InjectInfo::FillInjectCodeFilename(TCHAR* buf, const size_t numchars)
    {
        const size_t lengthBasePath = Globals::FillModuleBasePath(buf, numchars);
        
        if (0 == lengthBasePath)
            return false;

#if defined(HOOKSHOT64)
        const UINT extensionResourceID = IDS_HOOKSHOT_INJECT64_EXTENSION;
#else
        const UINT extensionResourceID = IDS_HOOKSHOT_INJECT32_EXTENSION;
#endif
        
        TCHAR extension[16];
        const size_t lengthExtension = LoadString(Globals::GetInstanceHandle(), extensionResourceID, extension, _countof(extension));

        if (0 == lengthExtension)
            return false;

        if ((lengthBasePath + lengthExtension) >= numchars)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return false;
        }

        _tcscpy_s(&buf[lengthBasePath], (1 + lengthExtension), extension);

        return true;
    }
}
