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
    namespace Globals
    {
        // -------- INTERNAL TYPES ----------------------------------------- //

        /// Holds all static data that falls under the global category.
        /// Used to make sure that globals are initialized as early as possible so that values are available during dynamic initialization.
        /// Implemented as a singleton object.
        class GlobalData
        {
        public:
            // -------- INSTANCE VARIABLES --------------------------------- //

            /// Pseudohandle of the current process.
            HANDLE gCurrentProcessHandle;

            /// PID of the current process.
            DWORD gCurrentProcessId;

            /// Handle of the instance that represents the running form of Hookshot.
            HINSTANCE gInstanceHandle;

            /// Method by which Hookshot was loaded into the current process.
            ELoadMethod gLoadMethod;

            /// Holds information about the current system, as retrieved from Windows.
            SYSTEM_INFO gSystemInformation;


        private:
            // -------- CONSTRUCTION AND DESTRUCTION ----------------------- //

            /// Default constructor.  Objects cannot be constructed externally.
            GlobalData(void) : gCurrentProcessHandle(GetCurrentProcess()), gCurrentProcessId(GetProcessId(GetCurrentProcess())), gInstanceHandle(nullptr), gLoadMethod(ELoadMethod::Executed), gSystemInformation()
            {
                GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&GlobalData::GetInstance, &gInstanceHandle);
                GetNativeSystemInfo(&gSystemInformation);
            }

            /// Copy constructor. Should never be invoked.
            GlobalData(const GlobalData& other) = delete;


        public:
            // -------- CLASS METHODS -------------------------------------- //

            /// Returns a reference to the singleton instance of this class.
            /// @return Reference to the singleton instance.
            static GlobalData& GetInstance(void)
            {
                static GlobalData globalData;
                return globalData;
            }
        };


        // -------- FUNCTIONS ---------------------------------------------- //
        // See "Globals.h" for documentation.

        HANDLE GetCurrentProcessHandle(void)
        {
            return GlobalData::GetInstance().gCurrentProcessHandle;
        }

        // --------

        DWORD GetCurrentProcessId(void)
        {
            return GlobalData::GetInstance().gCurrentProcessId;
        }

        // --------

        ELoadMethod GetHookshotLoadMethod(void)
        {
            return GlobalData::GetInstance().gLoadMethod;
        }

        // --------

        std::wstring_view GetHookshotLoadMethodString(void)
        {
            switch (GlobalData::GetInstance().gLoadMethod)
            {
            case ELoadMethod::Executed:
                return L"EXECUTED";

            case ELoadMethod::Injected:
                return L"INJECTED";

            case ELoadMethod::LibraryLoaded:
                return L"LIBRARY_LOADED";

            default:
                return L"UNKNOWN";
            }
        }

        // --------

        HINSTANCE GetInstanceHandle(void)
        {
            return GlobalData::GetInstance().gInstanceHandle;
        }

        // --------

        const SYSTEM_INFO& GetSystemInformation(void)
        {
            return GlobalData::GetInstance().gSystemInformation;
        }

        // --------

        void SetHookshotLoadMethod(const ELoadMethod loadMethod)
        {
            GlobalData::GetInstance().gLoadMethod = loadMethod;
        }
    }
}
