/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file ProcessInjector.h
 *   Interface declaration for process creation and injection.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"


namespace Hookshot
{
    /// Enumeration of possible error conditions that arise when attempting to create and inject a process.
    enum EInjectResult : int
    {
        // Success
        InjectResultSuccess = 0,                                    ///< All operations succeeded.
        
        // Issues creating the process
        InjectResultErrorCreateProcess = 1,                         ///< Call to `CreateProcess` failed.  `GetLastError` can be used to obtain more details.
        
        // Issues determining the base address of the process' executable image
        InjectResultErrorLoadNtDll = 2,                             ///< Attempt to dynamically load `ntdll.dll` failed.
        InjectResultErrorNtQueryInformationProcessUnavailable = 3,  ///< Attempt to locate `NtQueryInformationProcess` within `ntdll.dll` failed.
        InjectResultErrorNtQueryInformationProcessFailed = 4,       ///< Call to `NtQueryInformationProcess` failed to retrieve the desired information.
        InjectResultErrorReadProcessPEBFailed = 5,                  ///< Virtual memory read of the process PEB failed.

        // Issues determining the entry point address of a process
        InjectResultErrorReadDOSHeadersFailed = 6,                  ///< Failed to read DOS headers from the process' executable image.
        InjectResultErrorReadNTHeadersFailed = 7,                   ///< Failed to read NT headers from the process' executable image.
    };
    
    /// Provides all functionality related to creating processes and injecting them with Hookshot code.
    /// All methods are class methods.
    class ProcessInjector
    {
    private:
        // -------- CLASS VARIABLES ------------------------------------------------ //

        /// Module handle for "ntdll.dll" which is loaded dynamically as part of the process injection functionality.
        static HMODULE ntdllModuleHandle;

        /// Procedure address for the "NtQueryInformationProcess" function within "ntdll.dll" which is used to detect information about the newly-created process.
        static void* ntdllProcAddress;

        /// System allocation granularity.  Captured once and re-used as needed.
        static size_t systemAllocationGranularity;

        
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor. Should never be invoked.
        ProcessInjector(void) = delete;


    public:
        // -------- CLASS METHODS ------------------------------------------ //

        /// Creates a new process using the specified parameters and attempts to inject Hookshot code into it before it is allowed to run.
        /// This is essentially a macro for selecting between ANSI and wide-character versions.
        /// Refer to Microsoft's documentation on CreateProcess for information on parameters.
        /// @return Indictor of the result of the operation.
        static inline EInjectResult CreateInjectedProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
        {
#ifdef UNICODE
            return CreateInjectedProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
#else
            return CreateInjectedProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
#endif
        }
        
        /// Creates a new process using the specified parameters and attempts to inject Hookshot code into it before it is allowed to run.
        /// This is the ANSI version; refer to Microsoft's documentation on CreateProcessA for information on parameters.
        /// @return Indictor of the result of the operation.
        static EInjectResult CreateInjectedProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

        /// Creates a new process using the specified parameters and attempts to inject Hookshot code into it before it is allowed to run.
        /// This is the wide-character version; refer to Microsoft's documentation on CreateProcessW for information on parameters.
        /// @return Indictor of the result of the operation.
        static EInjectResult CreateInjectedProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);


    private:
        // -------- HELPERS ------------------------------------------------ //
        
        /// Retrieves and returns the system's virtual memory allocation granularity.
        /// @return Allocation granularity in bytes, or 0 to indicate a failure.
        static size_t GetSystemAllocationGranularity(void);
        
        /// Attempts to determine the address of the entry point of the given process.
        /// All addresses used by this method are in the virtual address space of the target process.
        /// @param [in] processHandle Handle to the process for which information is requested.
        /// @param [in] baseAddress Base address of the process' executable image.
        /// @param [out] entryPoint Address of the pointer that receives the entry point address.
        /// @return Indictor of the result of the operation.
        static EInjectResult GetProcessEntryPointAddress(const HANDLE processHandle, const void* const baseAddress, void** const entryPoint);
        
        /// Attempts to determine the base address of the primary executable image for the given process.
        /// All addresses used by this method are in the virtual address space of the target process.
        /// @param [in] processHandle Handle to the process for which information is requested.
        /// @param [out] baseAddress Address of the pointer that receives the image base address.
        /// @return Indictor of the result of the operation.
        static EInjectResult GetProcessImageBaseAddress(const HANDLE processHandle, void** const baseAddress);

        /// Common code to handle the result of an injection attempt.
        /// @param [in] result Injection attempt result.
        /// @param [in] shouldKeepSuspended Indicates whether or not the newly-created process should be kept in suspended state once injection is complete.
        /// @param [in] processHandle Handle to the newly-created process.
        /// @param [in] threadHandle Handle to the main thread of the newly-created process.
        /// @return Indictor of the result of the operation.
        static EInjectResult HandleInjectionResult(const EInjectResult result, const bool shouldKeepSuspended, const HANDLE processHandle, const HANDLE threadHandle);
        
        /// Attempts to inject a newly-created process with Hookshot code.
        /// @param [in] processHandle Handle to the newly-created process.
        /// @param [in] threadHandle Handle to the main thread of the newly-created process.
        /// @return Indictor of the result of the operation.
        static EInjectResult InjectNewlyCreatedProcess(const HANDLE processHandle, const HANDLE threadHandle);
    };
}
