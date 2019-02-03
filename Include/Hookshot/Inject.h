/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file Inject.h
 *   C++ interface to injection code written in assembly.
 *****************************************************************************/

#pragma once


// -------- TYPE DEFINITIONS ----------------------------------------------- //

/// Defines the structure of the data exchanged between the injecting and injected processes.
/// One instance of this structure is placed into the data region and accessed by both the injecting and injected processes.
/// A corresponding structure definition must appear in "Inject.inc" for the assembly code.
// TODO
struct SInjectData
{
    size_t unused;
};


// -------- EXTERNAL REFERENCES -------------------------------------------- //

// Required because names automatically get leading underscores in 32-bit mode.
#ifdef HOOKSHOT64
#define injectTrampolineStart               _injectTrampolineStart
#define injectTrampolineAddressMarker       _injectTrampolineAddressMarker
#define injectTrampolineEnd                 _injectTrampolineEnd
#define injectCodeStart                     _injectCodeStart
#define injectCodeBegin                     _injectCodeBegin
#define injectCodeEnd                       _injectCodeEnd
#endif

/// Start of the trampoline code block.
extern "C" void injectTrampolineStart(void);

/// Marker for where to place the address to which the trampoline should jump.
extern "C" void injectTrampolineAddressMarker(void);

/// End of the trampoline code block.
extern "C" void injectTrampolineEnd(void);

/// Start of the main code block.
extern "C" void injectCodeStart(void);

/// Entry point within the main code block.
extern "C" void injectCodeBegin(void);

/// End of the main code block.
extern "C" void injectCodeEnd(void);
