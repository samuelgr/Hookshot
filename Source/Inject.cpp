/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Inject.cpp
 *   Utility implementations for interfacing with injection code.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"
#include "Inject.h"
#include "InjectResult.h"
#include "StringUtilities.h"
#include "TemporaryBuffer.h"

#include <cstddef>
#include <cstring>


namespace Hookshot
{
    // -------- INTERNAL CONSTANTS ----------------------------------------- //

    /// Magic value that identifies the metadata section of a loaded binary file.
    static constexpr DWORD kInjectionMetaMagicValue = 0x51525354;


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


    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Obtains access to the binary data that contains the injection code.
    /// @param [out] baseAddress On success, filled with the base address of the injection code.
    /// @param [out] sizeBytes On success, filled with the size in bytes of the injection code.
    /// @return `true` on success, `false` on failure.
    static bool LoadInjectCodeBinary(void** baseAddress, size_t* sizeBytes)
    {
        static void* injectCodeBinaryBaseAddress = NULL;
        static size_t injectCodeBinarySizeBytes = 0;

        if (NULL == injectCodeBinaryBaseAddress)
        {
            const HRSRC resourceInfoBlock = FindResource(Globals::GetInstanceHandle(), MAKEINTRESOURCE(IDR_HOOKSHOT_INJECT_CODE), RT_RCDATA);
            if (NULL == resourceInfoBlock)
                return false;

            const HGLOBAL resourceHandle = LoadResource(Globals::GetInstanceHandle(), resourceInfoBlock);
            if (NULL == resourceHandle)
                return false;

            void* const resourceBaseAddress = LockResource(resourceHandle);
            if (NULL == resourceBaseAddress)
                return false;

            size_t resourceSizeBytes = (size_t)SizeofResource(Globals::GetInstanceHandle(), resourceInfoBlock);
            if (0 == resourceSizeBytes)
                return false;

            injectCodeBinaryBaseAddress = resourceBaseAddress;
            injectCodeBinarySizeBytes = resourceSizeBytes;
        }

        *baseAddress = injectCodeBinaryBaseAddress;
        *sizeBytes = injectCodeBinarySizeBytes;
        return true;
    }


    // -------- CONSTRUCTION AND DESTRUCTION ------------------------------- //
    // See "Inject.h" for documentation.

    InjectInfo::InjectInfo(void) : injectTrampolineStart(NULL), injectTrampolineAddressMarker(NULL), injectTrampolineEnd(NULL), injectCodeStart(NULL), injectCodeBegin(NULL), injectCodeEnd(NULL), initializationResult(EInjectResult::InjectResultFailure)
    {
        void* injectBinaryBase = NULL;
        size_t injectBinarySizeBytes = 0;

        if (false == LoadInjectCodeBinary(&injectBinaryBase, &injectBinarySizeBytes))
        {
            initializationResult = EInjectResult::InjectResultErrorCannotLoadInjectCode;
            return;
        }
        
        if (kMaxInjectBinaryFileSize < injectBinarySizeBytes)
        {
            initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
            return;
        }

        // Parse the injection binary and fill pointer values.
        {
            // Verify a valid DOS header.
            const IMAGE_DOS_HEADER* const dosHeader = (IMAGE_DOS_HEADER*)injectBinaryBase;
            
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

                        sectionCode = (void*)((size_t)injectBinaryBase + (size_t)sectionHeader[secidx].PointerToRawData);
                    }
                    else if (0 == memcmp((void*)Strings::kStrInjectMetaSectionName, (void*)&sectionHeader[secidx].Name, Strings::kLenInjectMetaSectionName * sizeof(Strings::kStrInjectMetaSectionName[0])))
                    {
                        if (NULL != sectionMeta)
                        {
                            initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                            return;
                        }

                        sectionMeta = (SInjectMeta*)((size_t)injectBinaryBase + (size_t)sectionHeader[secidx].PointerToRawData);
                    }
                }
            }

            // Verify that both sections exist.
            if ((NULL == sectionCode) || (NULL == sectionMeta))
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

            // Check the validity and version-correctness of the metadata section.
            if (kInjectionMetaMagicValue != sectionMeta->magic)
            {
                initializationResult = EInjectResult::InjectResultErrorMalformedInjectCodeFile;
                return;
            }

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
}
