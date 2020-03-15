/******************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ******************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2020
 **************************************************************************//**
 * @file Custom.cpp
 *   Test cases that follow the CUSTOM pattern.
 *****************************************************************************/

#include "CpuInfo.h"
#include "TestPattern.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <hookshot.h>
#include <tchar.h>
#include <windows.h>


// -------- MACROS --------------------------------------------------------- //

/// Generates a new function using #FunctionGenerator and returns a pointer to it.
/// A maximum of one instance of this macro can exist on each source code line.
#define GENERATE_FUNCTION()                 &FunctionGenerator<__LINE__>

/// Generates a new function using #FunctionGenerator and creates a pointer to it, using the specified name as the variable name.
/// A maximum of one instance of this macro can exist on each source code line.
#define GENERATE_AND_ASSIGN_FUNCTION(var)   const auto var = GENERATE_FUNCTION()


namespace HookshotTest
{
    // -------- INTERNAL TYPES --------------------------------------------- //

    /// Pointer-to-function type for the #FunctionGenerator function template.
    typedef int(*TGeneratedTestFunction)(void);


    // -------- INTERNAL FUNCTIONS ----------------------------------------- //

    /// Not intended ever to be called, but can be used to generate original and hook functions.
    template <int n> HOOKSHOT_TEST_HELPER_FUNCTION int FunctionGenerator(void)
    {
        const int val = 100 * n;

        for (int i = 0; i < (val / 10); ++i)
            srand((unsigned int)(val + i));

        return val;
    }


    // -------- TEST CASES ------------------------------------------------- //

    // Creates a hook chain going backwards.
    // Function B hooks function C (OK), then function A hooks function B (error).
    HOOKSHOT_CUSTOM_TEST(BackwardHookChain)
    {
        GENERATE_AND_ASSIGN_FUNCTION(funcA);
        GENERATE_AND_ASSIGN_FUNCTION(funcB);
        GENERATE_AND_ASSIGN_FUNCTION(funcC);

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(funcB, funcC)));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->CreateHook(funcA, funcB));

        HOOKSHOT_TEST_PASSED;
    }

    // Attempts to set the same hook twice.
    // Expected result is a failure due to the hook already existing.
    HOOKSHOT_CUSTOM_TEST(DuplicateHook)
    {
        GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
        GENERATE_AND_ASSIGN_FUNCTION(hookFunc);

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(originalFunc, hookFunc)));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->CreateHook(originalFunc, hookFunc));

        HOOKSHOT_TEST_PASSED;
    }

    // Creates a hook chain going forwards.
    // Function A hooks function B (OK), then function B hooks function C (error).
    HOOKSHOT_CUSTOM_TEST(ForwardHookChain)
    {
        GENERATE_AND_ASSIGN_FUNCTION(funcA);
        GENERATE_AND_ASSIGN_FUNCTION(funcB);
        GENERATE_AND_ASSIGN_FUNCTION(funcC);

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(funcA, funcB)));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->CreateHook(funcB, funcC));

        HOOKSHOT_TEST_PASSED;
    }

    // Creates a hook cycle.
    // Function A hooks function B (OK), then function B hooks function A (error).
    HOOKSHOT_CUSTOM_TEST(HookCycle)
    {
        GENERATE_AND_ASSIGN_FUNCTION(funcA);
        GENERATE_AND_ASSIGN_FUNCTION(funcB);

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(funcA, funcB)));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->CreateHook(funcB, funcA));

        HOOKSHOT_TEST_PASSED;
    }

    // Attempts to hook a Hookshot function.
    // Expected result is Hookshot rejects the input arguments as invalid.
    HOOKSHOT_CUSTOM_TEST(HookHookshot)
    {
        GENERATE_AND_ASSIGN_FUNCTION(hookFunc);
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailInvalidArgument == hookConfig->CreateHook(HookshotLibraryInitialize, hookFunc));
        HOOKSHOT_TEST_PASSED;
    }

    // Exercises Hookshot's concurrency control mechanisms by creating several threads and having them all set the same hooks.
    // Expect that exactly one hook-setting operation per original/hook pair will be reported successsful and, furthermore, function correctly.

    // Information structure to pass to each thread.
    struct SMultipleThreadsTestData
    {
        Hookshot::IHookConfig* hookConfig;
        const HookshotTest::ITestCase* testCase;

        HANDLE syncEventPhase1;
        HANDLE syncEventPhase2;
        volatile LONG syncThreadCounter;
        int numThreads;

        TGeneratedTestFunction* originalFuncs;
        TGeneratedTestFunction* hookFuncs;
        int numFuncs;
    };

    // Executed by each thread.
    DWORD WINAPI MultipleThreadsTestThreadProc(LPVOID lpParameter)
    {
        SMultipleThreadsTestData& testData = *(SMultipleThreadsTestData*)lpParameter;
        DWORD numSuccesses = 0;

        const int threadID = (int)InterlockedAdd(&testData.syncThreadCounter, 1);
        if (testData.numThreads == threadID)
            SetEvent(testData.syncEventPhase1);

        if (WAIT_OBJECT_0 != WaitForSingleObject(testData.syncEventPhase2, 10000))
            return 0;

        for (int i = 0; i < testData.numFuncs; ++i)
        {
            if (Hookshot::SuccessfulResult(testData.hookConfig->CreateHook(testData.originalFuncs[i], testData.hookFuncs[i])))
            {
                testData.testCase->PrintFormatted(_T("Thread %d: Successfully set hook at index %d."), threadID, i);
                numSuccesses += 1;
            }
        }

        return numSuccesses;
    }

    // Main test case logic.
    HOOKSHOT_CUSTOM_TEST(MultipleThreads)
    {
        SMultipleThreadsTestData testData;
        testData.hookConfig = hookConfig;
        testData.testCase = this;

        // Create and initialize synchronization objects.
        // This test uses a two-phase synchronization process.
        // First, the test case body waits for all threads to be initialized.  Threads increment a thread counter, and the last thread sets the phase 1 event.
        // Second, all threads wait for an event that is set by the test case body once it is woken up by the phase 1 event.
        // This is done to ensure that all threads start hammering Hookshot with requests at around the same time.
        testData.syncThreadCounter = 0;
        testData.syncEventPhase1 = CreateEvent(NULL, TRUE, FALSE, NULL);
        testData.syncEventPhase2 = CreateEvent(NULL, TRUE, FALSE, NULL);
        HOOKSHOT_TEST_ASSERT(NULL != testData.syncEventPhase1);
        HOOKSHOT_TEST_ASSERT(NULL != testData.syncEventPhase2);

        TGeneratedTestFunction originalFuncs[] = {
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION()
        };

        TGeneratedTestFunction hookFuncs[] = {
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION(),
            GENERATE_FUNCTION()
        };

        static_assert(_countof(originalFuncs) == _countof(hookFuncs), "MultipleThreads Test: number of original and hook functions must match.");
        static_assert(0 == _countof(originalFuncs) % 4, "MultipleThreads Test: number of functions must be divisible by 4.");

        testData.originalFuncs = originalFuncs;
        testData.hookFuncs = hookFuncs;
        testData.numFuncs = _countof(originalFuncs);

        constexpr int kNumThreads = _countof(originalFuncs) / 4;
        testData.numThreads = kNumThreads;
        PrintFormatted(_T("Creating %d threads."), kNumThreads);

        // Capture some expected values.
        // These are just the results of calling the hook functions.
        // Later, they will be compared to the result of calling the original function as a test of proper Hookshot behavior.
        int expectedValues[_countof(originalFuncs)];
        for (int i = 0; i < _countof(originalFuncs); ++i)
            expectedValues[i] = hookFuncs[i]();

        HANDLE threadHandles[kNumThreads];
        for (int i = 0; i < kNumThreads; ++i)
        {
            threadHandles[i] = CreateThread(NULL, 0, MultipleThreadsTestThreadProc, &testData, 0, NULL);
            HOOKSHOT_TEST_ASSERT(NULL != threadHandles[i]);
        }

        HOOKSHOT_TEST_ASSERT(WAIT_OBJECT_0 == WaitForSingleObject(testData.syncEventPhase1, 10000));
        HOOKSHOT_TEST_ASSERT(0 != SetEvent(testData.syncEventPhase2));
        HOOKSHOT_TEST_ASSERT(WAIT_OBJECT_0 == WaitForMultipleObjects(kNumThreads, threadHandles, TRUE, 10000));

        // At this point all threads have exited.
        // Each thread returned the number of successful attempts at setting a hook.
        // The expectation is that there was one successful operation per original/hook pair.
        // First, verify that the total number of successful attempts equals the expected number.
        // Second, verify that each original function was properly hooked.

        DWORD totalNumSuccesses = 0;
        for (int i = 0; i < kNumThreads; ++i)
        {
            DWORD thisThreadNumSuccesses = 0;
            HOOKSHOT_TEST_ASSERT(0 != GetExitCodeThread(threadHandles[i], &thisThreadNumSuccesses));

            CloseHandle(threadHandles[i]);

            totalNumSuccesses += thisThreadNumSuccesses;
        }

        PrintFormatted(_T("%d total successful hook set operations."), totalNumSuccesses);
        HOOKSHOT_TEST_ASSERT(_countof(originalFuncs) == totalNumSuccesses);

        for (int i = 0; i < _countof(originalFuncs); ++i)
        {
            const int actualValue = originalFuncs[i]();
            PrintFormatted(_T("Hook %d: %s: expected %d, got %d."), i, (actualValue == expectedValues[i] ? _T("OK") : _T("BAD")), expectedValues[i], actualValue);
            HOOKSHOT_TEST_ASSERT(actualValue == expectedValues[i]);
        }

        CloseHandle(testData.syncEventPhase1);
        CloseHandle(testData.syncEventPhase2);

        HOOKSHOT_TEST_PASSED;
    }

    // Hookshot is presented with two null pointers.
    // Expected result is Hookshot rejects the input arguments as invalid.
    HOOKSHOT_CUSTOM_TEST(NullPointerBoth)
    {
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailInvalidArgument == hookConfig->CreateHook(NULL, NULL));
        HOOKSHOT_TEST_PASSED;
    }

    // Hookshot is presented with a null pointer for the hook function.
    // Expected result is Hookshot rejects the input arguments as invalid.
    HOOKSHOT_CUSTOM_TEST(NullPointerHook)
    {
        GENERATE_AND_ASSIGN_FUNCTION(func);
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailInvalidArgument == hookConfig->CreateHook(func, NULL));
        HOOKSHOT_TEST_PASSED;
    }

    // Hookshot is presented with a null pointer for the original function.
    // Expected result is Hookshot rejects the input arguments as invalid.
    HOOKSHOT_CUSTOM_TEST(NullPointerOriginal)
    {
        GENERATE_AND_ASSIGN_FUNCTION(func);
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailInvalidArgument == hookConfig->CreateHook(NULL, func));
        HOOKSHOT_TEST_PASSED;
    }

    // Hookshot is presented with equal non-null pointers for both original and hook functions.
    // Expected result is Hookshot rejects the input arguments as invalid.
    HOOKSHOT_CUSTOM_TEST(SelfHook)
    {
        GENERATE_AND_ASSIGN_FUNCTION(func);
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailInvalidArgument == hookConfig->CreateHook(func, func));
        HOOKSHOT_TEST_PASSED;
    }

    // Attempts to replace a valid hook's hook function with one that is already involved in another hook.
    // Expected result is a failure due to a duplicate hook being detected.
    HOOKSHOT_CUSTOM_TEST(ReplaceHookDuplicate)
    {
        GENERATE_AND_ASSIGN_FUNCTION(originalFunc1);
        GENERATE_AND_ASSIGN_FUNCTION(hookFunc1);
        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(originalFunc1, hookFunc1)));

        GENERATE_AND_ASSIGN_FUNCTION(originalFunc2);
        GENERATE_AND_ASSIGN_FUNCTION(hookFunc2);
        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(originalFunc2, hookFunc2)));

        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->ReplaceHookFunction(originalFunc1, hookFunc2));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->ReplaceHookFunction(hookFunc1, hookFunc2));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->ReplaceHookFunction(originalFunc2, hookFunc1));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailDuplicate == hookConfig->ReplaceHookFunction(hookFunc2, hookFunc1));

        HOOKSHOT_TEST_PASSED;
    }

    // Attempts to replace a non-existent hook's hook function.
    // Expected result is Hookshot rejects the hook as not found.
    HOOKSHOT_CUSTOM_TEST(ReplaceHookNonExistent)
    {
        GENERATE_AND_ASSIGN_FUNCTION(funcA);
        GENERATE_AND_ASSIGN_FUNCTION(funcB);

        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailNotFound == hookConfig->ReplaceHookFunction(funcA, funcA));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailNotFound == hookConfig->ReplaceHookFunction(funcA, funcB));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailNotFound == hookConfig->ReplaceHookFunction(funcB, funcA));
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailNotFound == hookConfig->ReplaceHookFunction(funcB, funcB));

        HOOKSHOT_TEST_PASSED;
    }
    
    // Attempts to replace a valid hook's hook function with another valid hook function.
    // Verifies that Hookshot performs this operation both successfully and correctly.
    HOOKSHOT_CUSTOM_TEST(ReplaceHookValid)
    {
        GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
        GENERATE_AND_ASSIGN_FUNCTION(hookFunc);
        GENERATE_AND_ASSIGN_FUNCTION(replacementHookFunc);

        const auto kOriginalFuncResult = originalFunc();
        const auto kHookFuncResult = hookFunc();
        const auto kReplacementHookFuncResult = replacementHookFunc();

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(originalFunc, hookFunc)));
        HOOKSHOT_TEST_ASSERT(kHookFuncResult == originalFunc());
        HOOKSHOT_TEST_ASSERT(kOriginalFuncResult == ((decltype(originalFunc))hookConfig->GetOriginalFunction(originalFunc))());
        HOOKSHOT_TEST_ASSERT(NULL != hookConfig->GetOriginalFunction(hookFunc));

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->ReplaceHookFunction(hookFunc, replacementHookFunc)));
        HOOKSHOT_TEST_ASSERT(kReplacementHookFuncResult == originalFunc());
        HOOKSHOT_TEST_ASSERT(kOriginalFuncResult == ((decltype(originalFunc))hookConfig->GetOriginalFunction(originalFunc))());
        HOOKSHOT_TEST_ASSERT(NULL == hookConfig->GetOriginalFunction(hookFunc));
        HOOKSHOT_TEST_ASSERT(NULL != hookConfig->GetOriginalFunction(replacementHookFunc));

        HOOKSHOT_TEST_PASSED;
    }

    // Attempts to replace a valid hook's hook function with another valid hook function.
    // Expected result is success, even though the operation should have no effect.
    HOOKSHOT_CUSTOM_TEST(ReplaceHookWithSelf)
    {
        GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
        GENERATE_AND_ASSIGN_FUNCTION(hookFunc);

        const auto kOriginalFuncResult = originalFunc();
        const auto kHookFuncResult = hookFunc();

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->CreateHook(originalFunc, hookFunc)));
        HOOKSHOT_TEST_ASSERT(kHookFuncResult == originalFunc());
        HOOKSHOT_TEST_ASSERT(kOriginalFuncResult == ((decltype(originalFunc))hookConfig->GetOriginalFunction(originalFunc))());
        HOOKSHOT_TEST_ASSERT(NULL != hookConfig->GetOriginalFunction(hookFunc));

        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->ReplaceHookFunction(originalFunc, hookFunc)));
        HOOKSHOT_TEST_ASSERT(Hookshot::SuccessfulResult(hookConfig->ReplaceHookFunction(hookFunc, hookFunc)));
        HOOKSHOT_TEST_ASSERT(kHookFuncResult == originalFunc());
        HOOKSHOT_TEST_ASSERT(kOriginalFuncResult == ((decltype(originalFunc))hookConfig->GetOriginalFunction(originalFunc))());
        HOOKSHOT_TEST_ASSERT(NULL != hookConfig->GetOriginalFunction(hookFunc));

        HOOKSHOT_TEST_PASSED;
    }

    // Hookshot is presented with a valid original function but a hook function whose address is unsafely close to the original function.
    // Expected result is Hookshot rejects the input arguments as invalid.
    HOOKSHOT_CUSTOM_TEST(UnsafeSeparation)
    {
        GENERATE_AND_ASSIGN_FUNCTION(funcA);
        HOOKSHOT_TEST_ASSERT(Hookshot::EHookshotResult::HookshotResultFailInvalidArgument == hookConfig->CreateHook(funcA, (void*)((intptr_t)funcA + 1)));
        HOOKSHOT_TEST_PASSED;
    }
}
