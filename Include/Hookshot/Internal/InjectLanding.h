/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file InjectLanding.h
 *   Landing code C/C++ declarations.
 *   Receives control from injection code, cleans up, and runs the program.
 *****************************************************************************/

#pragma once

#include "Inject.h"


// -------- FUNCTIONS ------------------------------------------------------ //

/// Entry point for the landing code.
/// Written in assembly; actually a jump target, not a function.
/// When this DLL has been loaded and initialized successfully, control transfers to this address.
/// At the time that this occurs, this DLL assumes control of the injection and hook setup process, freeing the injecting process of all further responsibility.
/// There is no return, but rather upon completion control is transferred to the actual entry point of the injected process.
extern "C" void __fastcall InjectLanding(void);

/// Performs all necessary cleanup operations upon completion of the injection code.
/// @param [in] injectData Data used during the injection process.
extern "C" void __fastcall InjectLandingCleanup(const Hookshot::SInjectData* const injectData);

/// Performs all operations needed to read hook configuration information and load hook modules.
extern "C" void __fastcall InjectLandingLoadHookModules(const Hookshot::SInjectData* const injectData);
