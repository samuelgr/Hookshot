/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file InjectLanding.h
 *   Landing code C/C++ declarations.
 *   Receives control from injection code, cleans up, and runs the program.
 *****************************************************************************/

#pragma once

#include "ApiWindows.h"

#include <cstddef>


// -------- FORWARD DECLARATIONS ------------------------------------------- //
// See definitions for documentation.

namespace Hookshot
{
    struct SInjectData;
}


// -------- FUNCTIONS ------------------------------------------------------ //

/// Entry point for the landing code.
/// Written in assembly; actually a jump target, not a function.
/// When all injection operations are complete, the injection code jumps to this address.
/// It does not return, but rather transfers control to the actual entry point of the injected process.
extern "C" void APIENTRY InjectLanding(void);

/// Performs all necessary cleanup operations upon completion of the injection code.
/// @param [in] injectData Data used during the injection process.
extern "C" void APIENTRY InjectLandingCleanup(const Hookshot::SInjectData* const injectData);
