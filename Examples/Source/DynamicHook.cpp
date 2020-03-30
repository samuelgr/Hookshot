/******************************************************************************
 * Hookshot
 *   General-purpose library for hooking API calls in spawned processes.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019
 **************************************************************************//**
 * @file DynamicHook.cpp
 *   "DynamicHook" hook module example.
 *   This example shows how to use the dynamic hook types and definitions to
 *   create a hook more easily and safely than using Hookshot functions
 *   directly.  For the purpose of this example, the address of `MessageBoxW`
 *   is supplied at runtime.
 *****************************************************************************/

#include "Hookshot/DynamicHook.h"

#include <Windows.h>
