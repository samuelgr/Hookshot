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
#include "Strings.h"
#include "TemporaryBuffers.h"

#include <cstddef>
#include <cstring>


namespace Hookshot
{
    // -------- INTERNAL CONSTANTS ----------------------------------------- //

    /// Magic value that identifies the metadata section of a loaded binary file.
    static const DWORD kInjectionMetaMagicValue = 0x51525354;


    // -------- INTERNAL TYPES --------------------------------------------- //

    /// Defines the structure of the metadata section in a loaded binary file.
    struct SInjectMeta
    {
        // Fields present in all versions of Hookshot
        DWORD magic;                                                ///< Magic value.
        DWORD version;                                              ///< Version number.  Currently not used and must be 0.

        // Fields that may change from one version to the next.
        // These correspond to labels in the assembly-written code.
        // Each specifies an offset within the code section of the correspondingly-named label.
        DWORD offsetInjectTrampolineStart;
        DWORD offsetInjectTrampolineAddressMarker;
        DWORD offsetInjectTrampolineEnd;
        DWORD offsetInjectCodeStart;
        DWORD offsetInjectCodeBegin;
        DWORD offsetInjectCodeEnd;
    };


    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "Inject.h" for documentation.

    InjectInfo::InjectInfo(void) : injectTrampolineStart(NULL), injectTrampolineAddressMarker(NULL), injectTrampolineEnd(NULL), injectCodeStart(NULL), injectCodeBegin(NULL), injectCodeEnd(NULL), injectFileHandle(INVALID_HANDLE_VALUE), injectFileMappingHandle(INVALID_HANDLE_VALUE), injectFileBase(NULL), initializationResult(EInjectResult::InjectResultFailure)
    {
        // Figure out the name of the module that is to be loaded.
        TemporaryBuffer<TCHAR> moduleFilename;
        
        if (false == Strings::FillInjectBinaryFilename(moduleFilename, moduleFilename.count))
        {
            initializationResult = EInjectResult::InjectResultErrorCannotGenerateInjectCodeFilename;
            return;
        }

        // Attempt to open the module as a normal file.
        injectFileHandle = CreateFile(moduleFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if ((NULL == injectFileHandle) || (INVALID_HANDLE_VALUE == injectFileHandle))
        {
            injectFileHandle = INVALID_HANDLE_VALUE;
            initializationResult = EInjectResult::InjectResultErrorCannotLoadInjectCode;
            return;
        }

        // Verify the file size of the module.
        {
            LARGE_INTEGER fileSize;

            if ((FALSE == GetFileSizeEx(injectFileHandle, &fileSize)) || ((LONGLONG)kMaxInjectBinaryFileSize < fileSize.QuadPart))
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }
        }

        // Attempt to create a file mapping for the just-opened module file.
        injectFileMappingHandle = CreateFileMapping(injectFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        
        if ((NULL == injectFileMappingHandle) || (INVALID_HANDLE_VALUE == injectFileMappingHandle))
        {
            injectFileMappingHandle = INVALID_HANDLE_VALUE;
            initializationResult = EInjectResult::InjectResultErrorCannotLoadInjectCode;
            return;
        }

        // Attempt to map the module file into memory.
        injectFileBase = MapViewOfFile(injectFileMappingHandle, FILE_MAP_READ, 0, 0, 0);

        if (NULL == injectFileBase)
        {
            initializationResult = EInjectResult::InjectResultErrorCannotLoadInjectCode;
            return;
        }

        // Parse the mapped file and fill pointer values.
        {
            // Verify a valid DOS header.
            const IMAGE_DOS_HEADER* const dosHeader = (IMAGE_DOS_HEADER*)injectFileBase;
            
            if (IMAGE_DOS_SIGNATURE != dosHeader->e_magic)
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }
            
            // Verify a valid NT header and perform simple sanity checks.
            const IMAGE_NT_HEADERS* const ntHeader = (IMAGE_NT_HEADERS*)((size_t)dosHeader + (size_t)dosHeader->e_lfanew);
            
            if (IMAGE_NT_SIGNATURE != ntHeader->Signature)
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

#ifdef HOOKSHOT64
            if (IMAGE_FILE_MACHINE_AMD64 != ntHeader->FileHeader.Machine)
#else
            if (IMAGE_FILE_MACHINE_I386 != ntHeader->FileHeader.Machine)
#endif
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

            // Look through the section headers for the required code and metadata sections.
            void* sectionCode = NULL;
            SInjectMeta* sectionMeta = NULL;

            {
                const IMAGE_SECTION_HEADER* const sectionHeader = (IMAGE_SECTION_HEADER*)&ntHeader[1];

                // For each section found, check if its name matches one of the required section.
                // Since each such section should only appear once, also verify uniqueness.
                for (WORD secidx = 0; secidx < ntHeader->FileHeader.NumberOfSections; ++secidx)
                {
                    if (0 == memcmp((void*)Strings::kStrInjectCodeSectionName, (void*)&sectionHeader[secidx].Name, Strings::kLenInjectCodeSectionName * sizeof(Strings::kStrInjectCodeSectionName[0])))
                    {
                        if (NULL != sectionCode)
                        {
                            initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                            return;
                        }

                        sectionCode = (void*)((size_t)injectFileBase + (size_t)sectionHeader[secidx].PointerToRawData);
                    }
                    else if (0 == memcmp((void*)Strings::kStrInjectMetaSectionName, (void*)&sectionHeader[secidx].Name, Strings::kLenInjectMetaSectionName * sizeof(Strings::kStrInjectMetaSectionName[0])))
                    {
                        if (NULL != sectionMeta)
                        {
                            initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                            return;
                        }

                        sectionMeta = (SInjectMeta*)((size_t)injectFileBase + (size_t)sectionHeader[secidx].PointerToRawData);
                    }
                }
            }

            // Verify that both sections exist.
            if ((NULL == sectionCode) || (NULL == sectionMeta))
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

            // Check the validity and version-correctness (once implemented) of the metadata section.
            if (kInjectionMetaMagicValue != sectionMeta->magic)
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

            // Check the version code in the metadata section.
            // Currently not used and must be zero.
            if (0 != sectionMeta->version)
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

            // Fill in pointers using the information obtained.
            injectTrampolineStart = (void*)((size_t)sectionCode + (size_t)sectionMeta->offsetInjectTrampolineStart);
            injectTrampolineAddressMarker = (void*)((size_t)sectionCode + (size_t)sectionMeta->offsetInjectTrampolineAddressMarker);
            injectTrampolineEnd = (void*)((size_t)sectionCode + (size_t)sectionMeta->offsetInjectTrampolineEnd);
            injectCodeStart = (void*)((size_t)sectionCode + (size_t)sectionMeta->offsetInjectCodeStart);
            injectCodeBegin = (void*)((size_t)sectionCode + (size_t)sectionMeta->offsetInjectCodeBegin);
            injectCodeEnd = (void*)((size_t)sectionCode + (size_t)sectionMeta->offsetInjectCodeEnd);

            // All operations completed successfully.
            initializationResult = EInjectResult::InjectResultSuccess;
        }
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
}