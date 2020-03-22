/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Globals.cpp
 *   Implementation of accessors and mutators for global data items.
 *   Intended for miscellaneous data elements with no other suitable place.
 *****************************************************************************/

#include "ApiWindows.h"
#include "Globals.h"


namespace Hookshot
{
    // -------- INTERNAL TYPES --------------------------------------------- //

    /// Holds all static data that falls under the global category.
    /// Used to make sure that globals are initialized as early as possible so that values are available during dynamic initialization.
    /// Implemented as a singleton object.
    class GlobalData
    {
    public:
        // -------- INSTANCE VARIABLES ------------------------------------- //

        /// Handle of the instance that represents the running form of Hookshot.
        HINSTANCE gInstanceHandle;

        /// Method by which Hookshot was loaded into the current process.
        EHookshotLoadMethod gLoadMethod;


    private:
        // -------- CONSTRUCTION AND DESTRUCTION --------------------------- //

        /// Default constructor.  Objects cannot be constructed externally.
        GlobalData(void) : gInstanceHandle(NULL), gLoadMethod(EHookshotLoadMethod::Executed)
        {
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&GlobalData::GetInstance, &gInstanceHandle);
        }

        /// Copy constructor. Should never be invoked.
        GlobalData(const GlobalData& other) = delete;


    public:
        // -------- CLASS METHODS ------------------------------------------ //

        /// Returns a reference to the singleton instance of this class.
        /// @return Reference to the singleton instance.
        static GlobalData& GetInstance(void)
        {
            static GlobalData globalData;
            return globalData;
        }
    };


    // -------- CLASS METHODS ---------------------------------------------- //
    // See "Globals.h" for documentation.

    EHookshotLoadMethod Globals::GetHookshotLoadMethod(void)
    {
        return GlobalData::GetInstance().gLoadMethod;
    }

    // --------

    std::wstring_view Globals::GetHookshotLoadMethodString(void)
    {
        switch (GlobalData::GetInstance().gLoadMethod)
        {
        case EHookshotLoadMethod::Executed:
            return L"EXECUTED";

        case EHookshotLoadMethod::Injected:
            return L"INJECTED";

        case EHookshotLoadMethod::LibraryLoaded:
            return L"LIBRARY_LOADED";

        default:
            return L"UNKNOWN";
        }
    }

    // --------

    HINSTANCE Globals::GetInstanceHandle(void)
    {
        return GlobalData::GetInstance().gInstanceHandle;
    }

    // --------

    void Globals::SetHookshotLoadMethod(const EHookshotLoadMethod loadMethod)
    {
        GlobalData::GetInstance().gLoadMethod = loadMethod;
    }
}
