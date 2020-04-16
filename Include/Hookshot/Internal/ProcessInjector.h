/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file ProcessInjector.h
 *   Interface declaration for process creation and injection.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"
#include "InjectResult.h"
#include "RemoteProcessInjector.h"

#include <cstddef>


namespace Hookshot
{
    /// Provides all functionality related to creating processes and injecting them with Hookshot code.
    namespace ProcessInjector
    {
        // -------- FUNCTIONS ---------------------------------------------- //

        /// Creates a new process using the specified parameters and attempts to inject Hookshot code into it before it is allowed to run.
        /// Refer to Microsoft's documentation on CreateProcess for information on parameters.
        /// @return Indicator of the result of the operation.
        EInjectResult CreateInjectedProcess(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

        /// Injects a process created by another instance of Hookshot.
        /// Communication between instances occurs by means of shared memory using the handle passed in, including more detailed error information than is directly returned.
        /// @param [in,out] remoteInjectionData Data structure holding information exchanged between this Hookshot process and the Hookshot process that requested injection.
        /// @return `false` if an inter-process communication mechanism failed, `true` otherwise.
        bool PerformRequestedRemoteInjection(RemoteProcessInjector::SInjectRequest* const remoteInjectionData);
    }
}
