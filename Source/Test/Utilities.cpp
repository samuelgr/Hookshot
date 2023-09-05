/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2023
 *************************************************************************//**
 * @file Utilities.cpp
 *   Implementation of test utility functions.
 **************************************************************************************************/

#include <cstdarg>
#include <cstdio>
#include <sal.h>
#include <windows.h>

namespace HookshotTest
{
        

    void Print(const wchar_t* const str)
    {
        if (IsDebuggerPresent())
        {
            OutputDebugString(str);
            OutputDebugString(L"\n");
        }
        else
        {
            _putws(str);
        }
    }

    
    void PrintFormatted(_Printf_format_string_  const wchar_t* const format, ...)
    {
        wchar_t formattedStringBuffer[1024];

        va_list args;
        va_start(args, format);
        vswprintf_s(formattedStringBuffer, _countof(formattedStringBuffer), format, args);
        va_end(args);

        Print(formattedStringBuffer);
    }
}
