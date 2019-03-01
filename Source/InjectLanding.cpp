/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file InjectLanding.cpp
 *   Partial landing code implementation.
 *   Receives control from injection code, cleans up, and runs the program.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Inject.h"
#include "Message.h"

#include <cstddef>

using namespace Hookshot;


// -------- FUNCTIONS ------------------------------------------------------ //
// See "InjectLanding.h" for documentation.

extern "C" void APIENTRY InjectLandingCleanup(const SInjectData* const injectData)
{
    // Copy the array of addresses to free.
    // This is needed because freeing any one of these could invalidate the injectData pointer.
    void* cleanupBaseAddress[_countof(injectData->cleanupBaseAddress)];
    memcpy((void*)cleanupBaseAddress, (void*)injectData->cleanupBaseAddress, sizeof(cleanupBaseAddress));
    
    // Free all requested buffers.
    for (size_t i = 0; i < _countof(cleanupBaseAddress); ++i)
    {
        if (NULL != cleanupBaseAddress[i])
            VirtualFree(cleanupBaseAddress[i], 0, MEM_RELEASE);
    }
}

// --------

extern "C" void APIENTRY InjectLandingSetHooks(void)
{
#ifdef HOOKSHOT_DEBUG
    // For debugging purposes emit a message indicating the current process ID to facilitate attaching a debugger.
    if (FALSE == IsDebuggerPresent())
        Message::OutputFormattedFromResource(EMessageSeverity::MessageSeverityInfo, IDS_HOOKSHOT_DEBUG_PID_FORMAT, GetProcessId(GetCurrentProcess()));
#endif
}
