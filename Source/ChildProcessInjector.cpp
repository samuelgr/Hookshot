/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file ChildProcessInjector.cpp
 *   Implementation of internal hooks for injecting child processes.
 *****************************************************************************/

#include "DependencyProtect.h"
#include "InjectResult.h"
#include "InternalHook.h"
#include "Message.h"
#include "RemoteProcessInjector.h"
#include "TemporaryBuffer.h"



namespace Hookshot
{
    // -------- INTERNAL TYPES --------------------------------------------- //
    // These are the hook declarations for the CreateProcess family of functions.

    HOOKSHOT_INTERNAL_HOOK(CreateProcessA);
    HOOKSHOT_INTERNAL_HOOK(CreateProcessW);

    
    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Injects a newly-created child process with HookshotDll.
    /// Outputs a message indicating the result of the attempted injection.
    /// @param [in] processHandle Handle to the process to inject.
    /// @param [in] threadHandle Handle to the main thread of the process to inject.
    static void InjectChildProcess(const HANDLE processHandle, const HANDLE threadHandle)
    {
        TemporaryBuffer<wchar_t> childProcessExecutable;
        DWORD childProcessExecutableLength = childProcessExecutable.Count();

        if (0 == Protected::Windows_QueryFullProcessImageName(processHandle, 0, childProcessExecutable, &childProcessExecutableLength))
            childProcessExecutableLength = 0;

        const EInjectResult result = RemoteProcessInjector::InjectProcess(processHandle, threadHandle, false, Protected::Windows_IsDebuggerPresent());

        if (EInjectResult::InjectResultSuccess == result)
            Message::OutputFormatted(Message::ESeverity::Info, L"Successfully injected child process %s.", (0 == childProcessExecutableLength ? L"(error determining executable file name)" : &childProcessExecutable[0]));
        else
            Message::OutputFormatted(Message::ESeverity::Warning, L"EInjectResult %d.%d - Failed to inject child process %s.", (int)result, (int)GetLastError(), (0 == childProcessExecutableLength ? L"(error determining executable file name)" : &childProcessExecutable[0]));
    }


    // -------- FUNCTIONS -------------------------------------------------- //
    // These are the hook functions for the CreateProcess family of functions.

    BOOL InternalHook_CreateProcessA::Hook(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
    {
        const bool shouldCreateSuspended = (0 != (dwCreationFlags & CREATE_SUSPENDED)) ? true : false;
        PROCESS_INFORMATION processInfo = *lpProcessInformation;

        const BOOL createProcessResult = Original(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, (dwCreationFlags | CREATE_SUSPENDED), lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInfo);
        *lpProcessInformation = processInfo;

        if (0 != createProcessResult)
            InjectChildProcess(processInfo.hProcess, processInfo.hThread);

        if (false == shouldCreateSuspended)
            Protected::Windows_ResumeThread(processInfo.hThread);

        return createProcessResult;
    }

    // --------

    BOOL InternalHook_CreateProcessW::Hook(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
    {
        const bool shouldCreateSuspended = (0 != (dwCreationFlags & CREATE_SUSPENDED)) ? true : false;
        PROCESS_INFORMATION processInfo = *lpProcessInformation;

        const BOOL createProcessResult = Original(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, (dwCreationFlags | CREATE_SUSPENDED), lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInfo);
        *lpProcessInformation = processInfo;

        if (0 != createProcessResult)
            InjectChildProcess(processInfo.hProcess, processInfo.hThread);

        if (false == shouldCreateSuspended)
            Protected::Windows_ResumeThread(processInfo.hThread);

        return createProcessResult;
    }
}
