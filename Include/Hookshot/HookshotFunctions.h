/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file HookshotFunctions.h
 *   Function prototypes and macros for the public Hookshot interface.
 *   External users should include Hookshot.h instead of this file.
 *****************************************************************************/

#pragma once

#include "HookshotTypes.h"

#include <cstddef>
#include <cstdint>
#include <string_view>


/// Initializes the Hookshot library.
/// Hook modules do not need to invoke this function because Hookshot initializes itself before loading them.
/// If linking directly against the Hookshot library, this function must be invoked before other Hookshot functions.
/// The returned Hookshot interface pointer remains valid throughout the lifetime of the process and owned by Hookshot.
/// @return Hookshot interface pointer on success, or `nullptr` on failure.
extern "C" __declspec(dllimport) Hookshot::IHookshot* __fastcall HookshotLibraryInitialize(void);


/// Convenient definition for the entry point of a Hookshot hook module.
/// If building a hook module, use this macro to create a hook module entry point.
/// Macro parameter is the desired name of the entry point's function parameter, namely the Hookshot interface object pointer.
#define HOOKSHOT_HOOK_MODULE_ENTRY(param)   extern "C" __declspec(dllexport) void __fastcall HookshotMain(Hookshot::IHookshot* param)
