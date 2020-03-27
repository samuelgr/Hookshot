/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file ApiWindows.h
 *   Common header file for the correct version of the Windows API.
 *****************************************************************************/

#pragma once


// -------- WINDOWS API ---------------------------------------------------- //

#define NOMINMAX

#include <sdkddkver.h>
#include <windows.h>
