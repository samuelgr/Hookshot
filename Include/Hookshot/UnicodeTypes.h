/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file ApiUnicode.h
 *   Common definitions and helpers for adapting between Unicode encodings.
 *****************************************************************************/

#pragma once

#include <sstream>
#include <string>
#include <string_view>


// Type aliases for various standard string types.
// Underlying type depends on the selected underlying character representation.
#ifdef UNICODE

typedef std::wstringstream TStdStringStream;
typedef std::wstring TStdString;
typedef std::wstring_view TStdStringView;

#else

typedef std::stringstream TStdStringStream;
typedef std::string TSStdtring;
typedef std::string_view TSStdtringView;

#endif
