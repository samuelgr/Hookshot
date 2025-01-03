/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2025
 ***********************************************************************************************//**
 * @file Custom.cpp
 *   Test cases that follow the CUSTOM pattern.
 **************************************************************************************************/

#include <windows.h>

#include <Infra/Test/Utilities.h>

#include "Hookshot.h"
#include "TestGlobals.h"
#include "TestPattern.h"

/// Generates a new function using FunctionGenerator and returns a pointer to it.
/// A maximum of one instance of this macro can exist on each source code line.
#define GENERATE_FUNCTION()               &FunctionGenerator<__LINE__>

/// Generates a new function using FunctionGenerator and creates a pointer to it, using the
/// specified name as the variable name. A maximum of one instance of this macro can exist on each
/// source code line.
#define GENERATE_AND_ASSIGN_FUNCTION(var) const auto var = GENERATE_FUNCTION()

namespace HookshotTest
{
  /// Pointer-to-function type for the FunctionGenerator function template.
  using TGeneratedTestFunction = int (*)(void);

  /// Not intended ever to be called, but can be used to generate original and hook functions.
  template <int n> HOOKSHOT_TEST_HELPER_FUNCTION int FunctionGenerator(void)
  {
    const int val = 100 * n;

    for (int i = 0; i < (val / 10); ++i)
      srand(static_cast<unsigned int>(val + i));

    return val;
  }

  // Creates a hook chain going backwards.
  // Function B hooks function C (OK), then function A hooks function B (error).
  HOOKSHOT_CUSTOM_TEST(BackwardHookChain)
  {
    GENERATE_AND_ASSIGN_FUNCTION(funcA);
    GENERATE_AND_ASSIGN_FUNCTION(funcB);
    GENERATE_AND_ASSIGN_FUNCTION(funcC);

    TEST_ASSERT(Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(funcB, funcC)));
    TEST_ASSERT(Hookshot::EResult::FailDuplicate == HookshotInterface()->CreateHook(funcA, funcB));
  }

  // Attempts to set the same hook twice.
  // Expected result is a failure due to the hook already existing.
  HOOKSHOT_CUSTOM_TEST(DuplicateHook)
  {
    GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc);

    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc, hookFunc)));
    TEST_ASSERT(
        Hookshot::EResult::FailDuplicate ==
        HookshotInterface()->CreateHook(originalFunc, hookFunc));
  }

  // Disables and re-enables a hook.
  // Verifies that all operations are completed successfully and correctly.
  HOOKSHOT_CUSTOM_TEST(DisableAndReEnableHook)
  {
    GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc);

    const auto originalFuncResult = originalFunc();
    const auto hookFuncResult = hookFunc();

    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc, hookFunc)));
    TEST_ASSERT(hookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());

    TEST_ASSERT(Hookshot::SuccessfulResult(HookshotInterface()->DisableHookFunction(hookFunc)));
    TEST_ASSERT(Hookshot::SuccessfulResult(HookshotInterface()->DisableHookFunction(originalFunc)));
    TEST_ASSERT(originalFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
    TEST_ASSERT(nullptr == HookshotInterface()->GetOriginalFunction(hookFunc));

    TEST_ASSERT(
        Hookshot::EResult::FailNotFound ==
        HookshotInterface()->ReplaceHookFunction(hookFunc, hookFunc));
    TEST_ASSERT(Hookshot::SuccessfulResult(
        HookshotInterface()->ReplaceHookFunction(originalFunc, hookFunc)));
    TEST_ASSERT(hookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
  }

  // Creates a hook chain going forwards.
  // Function A hooks function B (OK), then function B hooks function C (error).
  HOOKSHOT_CUSTOM_TEST(ForwardHookChain)
  {
    GENERATE_AND_ASSIGN_FUNCTION(funcA);
    GENERATE_AND_ASSIGN_FUNCTION(funcB);
    GENERATE_AND_ASSIGN_FUNCTION(funcC);

    TEST_ASSERT(Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(funcA, funcB)));
    TEST_ASSERT(Hookshot::EResult::FailDuplicate == HookshotInterface()->CreateHook(funcB, funcC));
  }

  // Creates a hook cycle.
  // Function A hooks function B (OK), then function B hooks function A (error).
  HOOKSHOT_CUSTOM_TEST(HookCycle)
  {
    GENERATE_AND_ASSIGN_FUNCTION(funcA);
    GENERATE_AND_ASSIGN_FUNCTION(funcB);

    TEST_ASSERT(Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(funcA, funcB)));
    TEST_ASSERT(Hookshot::EResult::FailDuplicate == HookshotInterface()->CreateHook(funcB, funcA));
  }

  // Attempts to hook a Hookshot function.
  // Expected result is Hookshot rejects the input arguments as invalid.
  HOOKSHOT_CUSTOM_TEST(HookHookshot)
  {
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc);
    TEST_ASSERT(
        Hookshot::EResult::FailInvalidArgument ==
        HookshotInterface()->CreateHook(HookshotLibraryInitialize, hookFunc));
  }

  // Attempts to set a very high number of hooks.
  // Exercises Hookshot's data structure capacity.
  // Expected result is success on all fronts.
  HOOKSHOT_CUSTOM_TEST(ManyManyHooks)
  {
    // Each generated function must appear on its own line.

    // clang-format off

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
        GENERATE_FUNCTION(),
        GENERATE_FUNCTION(),
        GENERATE_FUNCTION(),
        GENERATE_FUNCTION(),
        GENERATE_FUNCTION()
    };

    // clang-format on

    static_assert(
        _countof(originalFuncs) == _countof(hookFuncs),
        "ManyManyHooks Test: number of original and hook functions must match.");

    for (int i = 0; i < _countof(originalFuncs); ++i)
      TEST_ASSERT(Hookshot::SuccessfulResult(
          HookshotInterface()->CreateHook(originalFuncs[i], hookFuncs[i])));
  }

  // Exercises Hookshot's concurrency control mechanisms by creating several threads and having them
  // all set the same hooks. Expect that exactly one hook-setting operation per original/hook pair
  // will be reported successsful and, furthermore, function correctly.

  // Information structure to pass to each thread.
  struct SMultipleThreadsTestData
  {
    const ::Infra::Test::ITestCase* testCase;
    Hookshot::IHookshot* hookshot;

    HANDLE syncEventPhase1;
    HANDLE syncEventPhase2;
    LONG syncThreadCounter;
    int numThreads;

    TGeneratedTestFunction* originalFuncs;
    TGeneratedTestFunction* hookFuncs;
    int numFuncs;
  };

  // Executed by each thread.
  DWORD WINAPI MultipleThreadsTestThreadProc(LPVOID lpParameter)
  {
    SMultipleThreadsTestData& testData = *reinterpret_cast<SMultipleThreadsTestData*>(lpParameter);
    DWORD numSuccesses = 0;

    const int threadID = static_cast<int>(InterlockedAdd(&testData.syncThreadCounter, 1));
    if (testData.numThreads == threadID) SetEvent(testData.syncEventPhase1);

    if (WAIT_OBJECT_0 != WaitForSingleObject(testData.syncEventPhase2, 10000)) return 0;

    for (int i = 0; i < testData.numFuncs; ++i)
    {
      if (Hookshot::SuccessfulResult(
              testData.hookshot->CreateHook(testData.originalFuncs[i], testData.hookFuncs[i])))
      {
        Infra::Test::PrintFormatted(L"Thread %d: Successfully set hook at index %d.", threadID, i);
        numSuccesses += 1;
      }
    }

    return numSuccesses;
  }

  // Main test case logic.
  HOOKSHOT_CUSTOM_TEST(MultipleThreads)
  {
    SMultipleThreadsTestData testData{.testCase = this, .hookshot = HookshotInterface()};

    // Create and initialize synchronization objects.
    // This test uses a two-phase synchronization process.
    // First, the test case body waits for all threads to be initialized. Threads increment a thread
    // counter, and the last thread sets the phase 1 event. Second, all threads wait for an event
    // that is set by the test case body once it is woken up by the phase 1 event. This is done to
    // ensure that all threads start hammering Hookshot with requests at around the same time.
    testData.syncThreadCounter = 0;
    testData.syncEventPhase1 = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    testData.syncEventPhase2 = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    TEST_ASSERT(nullptr != testData.syncEventPhase1);
    TEST_ASSERT(nullptr != testData.syncEventPhase2);

    // Each generated function must appear on its own line.

    // clang-format off

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
        GENERATE_FUNCTION()};

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
        GENERATE_FUNCTION()};

    // clang-format on

    static_assert(
        _countof(originalFuncs) == _countof(hookFuncs),
        "MultipleThreads Test: number of original and hook functions must match.");
    static_assert(
        0 == _countof(originalFuncs) % 4,
        "MultipleThreads Test: number of functions must be divisible by 4.");

    testData.originalFuncs = originalFuncs;
    testData.hookFuncs = hookFuncs;
    testData.numFuncs = _countof(originalFuncs);

    constexpr int kNumThreads = _countof(originalFuncs) / 4;
    testData.numThreads = kNumThreads;
    Infra::Test::PrintFormatted(L"Creating %d threads.", kNumThreads);

    // Capture some expected values.
    // These are just the results of calling the hook functions.
    // Later, they will be compared to the result of calling the original function as a test of
    // proper Hookshot behavior.
    int expectedValues[_countof(originalFuncs)];
    for (int i = 0; i < _countof(originalFuncs); ++i)
      expectedValues[i] = hookFuncs[i]();

    HANDLE threadHandles[kNumThreads];
    for (int i = 0; i < kNumThreads; ++i)
    {
      threadHandles[i] =
          CreateThread(nullptr, 0, MultipleThreadsTestThreadProc, &testData, 0, nullptr);
      TEST_ASSERT(nullptr != threadHandles[i]);
    }

    TEST_ASSERT(WAIT_OBJECT_0 == WaitForSingleObject(testData.syncEventPhase1, 10000));
    TEST_ASSERT(0 != SetEvent(testData.syncEventPhase2));
    TEST_ASSERT(WAIT_OBJECT_0 == WaitForMultipleObjects(kNumThreads, threadHandles, TRUE, 10000));

    // At this point all threads have exited.
    // Each thread returned the number of successful attempts at setting a hook.
    // The expectation is that there was one successful operation per original/hook pair.
    // First, verify that the total number of successful attempts equals the expected number.
    // Second, verify that each original function was properly hooked.

    DWORD totalNumSuccesses = 0;
    for (int i = 0; i < kNumThreads; ++i)
    {
      DWORD thisThreadNumSuccesses = 0;
      TEST_ASSERT(0 != GetExitCodeThread(threadHandles[i], &thisThreadNumSuccesses));

      CloseHandle(threadHandles[i]);

      totalNumSuccesses += thisThreadNumSuccesses;
    }

    Infra::Test::PrintFormatted(L"%d total successful hook set operations.", totalNumSuccesses);
    TEST_ASSERT(_countof(originalFuncs) == totalNumSuccesses);

    for (int i = 0; i < _countof(originalFuncs); ++i)
    {
      const int actualValue = originalFuncs[i]();
      Infra::Test::PrintFormatted(
          L"Hook %d: %s: expected %d, got %d.",
          i,
          (actualValue == expectedValues[i] ? L"OK" : L"BAD"),
          expectedValues[i],
          actualValue);
      TEST_ASSERT(actualValue == expectedValues[i]);
    }

    CloseHandle(testData.syncEventPhase1);
    CloseHandle(testData.syncEventPhase2);
  }

  // Hookshot is presented with two null pointers.
  // Expected result is Hookshot rejects the input arguments as invalid.
  HOOKSHOT_CUSTOM_TEST(NullPointerBoth)
  {
    TEST_ASSERT(
        Hookshot::EResult::FailInvalidArgument ==
        HookshotInterface()->CreateHook(nullptr, nullptr));
  }

  // Hookshot is presented with a null pointer for the hook function.
  // Expected result is Hookshot rejects the input arguments as invalid.
  HOOKSHOT_CUSTOM_TEST(NullPointerHook)
  {
    GENERATE_AND_ASSIGN_FUNCTION(func);
    TEST_ASSERT(
        Hookshot::EResult::FailInvalidArgument == HookshotInterface()->CreateHook(func, nullptr));
  }

  // Hookshot is presented with a null pointer for the original function.
  // Expected result is Hookshot rejects the input arguments as invalid.
  HOOKSHOT_CUSTOM_TEST(NullPointerOriginal)
  {
    GENERATE_AND_ASSIGN_FUNCTION(func);
    TEST_ASSERT(
        Hookshot::EResult::FailInvalidArgument == HookshotInterface()->CreateHook(nullptr, func));
  }

  // Hookshot is presented with equal non-null pointers for both original and hook functions.
  // Expected result is Hookshot rejects the input arguments as invalid.
  HOOKSHOT_CUSTOM_TEST(SelfHook)
  {
    GENERATE_AND_ASSIGN_FUNCTION(func);
    TEST_ASSERT(
        Hookshot::EResult::FailInvalidArgument == HookshotInterface()->CreateHook(func, func));
  }

  // Attempts to replace a valid hook's hook function with one that is already involved in another
  // hook. Expected result is a failure due to a duplicate hook being detected.
  HOOKSHOT_CUSTOM_TEST(ReplaceHookDuplicate)
  {
    GENERATE_AND_ASSIGN_FUNCTION(originalFunc1);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc1);
    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc1, hookFunc1)));

    GENERATE_AND_ASSIGN_FUNCTION(originalFunc2);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc2);
    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc2, hookFunc2)));

    TEST_ASSERT(
        Hookshot::EResult::FailDuplicate ==
        HookshotInterface()->ReplaceHookFunction(originalFunc1, hookFunc2));
    TEST_ASSERT(
        Hookshot::EResult::FailDuplicate ==
        HookshotInterface()->ReplaceHookFunction(hookFunc1, hookFunc2));
    TEST_ASSERT(
        Hookshot::EResult::FailDuplicate ==
        HookshotInterface()->ReplaceHookFunction(originalFunc2, hookFunc1));
    TEST_ASSERT(
        Hookshot::EResult::FailDuplicate ==
        HookshotInterface()->ReplaceHookFunction(hookFunc2, hookFunc1));
  }

  // Attempts to replace a non-existent hook's hook function.
  // Expected result is Hookshot rejects the hook as not found.
  HOOKSHOT_CUSTOM_TEST(ReplaceHookNonExistent)
  {
    GENERATE_AND_ASSIGN_FUNCTION(funcA);
    GENERATE_AND_ASSIGN_FUNCTION(funcB);

    TEST_ASSERT(
        Hookshot::EResult::FailNotFound == HookshotInterface()->ReplaceHookFunction(funcA, funcA));
    TEST_ASSERT(
        Hookshot::EResult::FailNotFound == HookshotInterface()->ReplaceHookFunction(funcA, funcB));
    TEST_ASSERT(
        Hookshot::EResult::FailNotFound == HookshotInterface()->ReplaceHookFunction(funcB, funcA));
    TEST_ASSERT(
        Hookshot::EResult::FailNotFound == HookshotInterface()->ReplaceHookFunction(funcB, funcB));
  }

  // Attempts to replace a valid hook's hook function with another valid hook function.
  // Verifies that Hookshot performs this operation both successfully and correctly.
  HOOKSHOT_CUSTOM_TEST(ReplaceHookValid)
  {
    GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc);
    GENERATE_AND_ASSIGN_FUNCTION(replacementHookFunc);

    const auto originalFuncResult = originalFunc();
    const auto hookFuncResult = hookFunc();
    const auto replacementHookFuncResult = replacementHookFunc();

    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc, hookFunc)));
    TEST_ASSERT(hookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
    TEST_ASSERT(nullptr != HookshotInterface()->GetOriginalFunction(hookFunc));

    TEST_ASSERT(Hookshot::SuccessfulResult(
        HookshotInterface()->ReplaceHookFunction(hookFunc, replacementHookFunc)));
    TEST_ASSERT(replacementHookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
    TEST_ASSERT(nullptr == HookshotInterface()->GetOriginalFunction(hookFunc));
    TEST_ASSERT(nullptr != HookshotInterface()->GetOriginalFunction(replacementHookFunc));
  }

  // Attempts to replace a valid hook's hook function with another valid hook function.
  // Expected result is success, even though the operation should have no effect.
  HOOKSHOT_CUSTOM_TEST(ReplaceHookWithSelf)
  {
    GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc);

    const auto originalFuncResult = originalFunc();
    const auto hookFuncResult = hookFunc();

    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc, hookFunc)));
    TEST_ASSERT(hookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
    TEST_ASSERT(nullptr != HookshotInterface()->GetOriginalFunction(hookFunc));

    TEST_ASSERT(Hookshot::SuccessfulResult(
        HookshotInterface()->ReplaceHookFunction(originalFunc, hookFunc)));
    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->ReplaceHookFunction(hookFunc, hookFunc)));
    TEST_ASSERT(hookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
    TEST_ASSERT(nullptr != HookshotInterface()->GetOriginalFunction(hookFunc));
  }

  // Hookshot is presented with a valid original function but a hook function whose address is
  // unsafely close to the original function. Expected result is Hookshot rejects the input
  // arguments as invalid.
  HOOKSHOT_CUSTOM_TEST(UnsafeSeparation)
  {
    GENERATE_AND_ASSIGN_FUNCTION(funcA);
    TEST_ASSERT(
        Hookshot::EResult::FailInvalidArgument ==
        HookshotInterface()->CreateHook(
            funcA, reinterpret_cast<void*>(reinterpret_cast<intptr_t>(funcA) + 1)));
  }

  // Performs a standard hooking operation after hooking some Windows API functions used internally
  // by Hookshot to implement hooking. Hookshot should be resilient against this and work anyway.
  // Hook version of the VirtualProtect Windows API function. Always fails.
  BOOL WINAPI HookVirtualProtect(
      LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
  {
    return FALSE;
  }

  // Resolves the correct address to use for the original VirtualProtect function.
  // If this function exists in the low-level binary KernelBase.dll, get it from there.
  // If not, obtain its address statically.
  static void* GetVirtualProtectAddress(void)
  {
    HMODULE hmodKernelBase = LoadLibrary(L"KernelBase.dll");
    if (nullptr == hmodKernelBase) return &VirtualProtect;

    void* const virtualProtectKernelBase = GetProcAddress(hmodKernelBase, "VirtualProtect");
    if (nullptr == virtualProtectKernelBase) return &VirtualProtect;

    return virtualProtectKernelBase;
  }

  // Main test case logic.
  HOOKSHOT_CUSTOM_TEST(WindowsApiUsedByHookshot)
  {
    // Hook the VirtualProtect Windows API function.
    // This is used internally by Hookshot to set memory permissions while setting hooks.
    TEST_ASSERT(Hookshot::SuccessfulResult(
        HookshotInterface()->CreateHook(GetVirtualProtectAddress(), &HookVirtualProtect)));

    // Standard hook operations follow. These are expected to succeed.
    GENERATE_AND_ASSIGN_FUNCTION(originalFunc);
    GENERATE_AND_ASSIGN_FUNCTION(hookFunc);

    const auto originalFuncResult = originalFunc();
    const auto hookFuncResult = hookFunc();

    TEST_ASSERT(
        Hookshot::SuccessfulResult(HookshotInterface()->CreateHook(originalFunc, hookFunc)));
    TEST_ASSERT(hookFuncResult == originalFunc());
    TEST_ASSERT(
        originalFuncResult ==
        ((decltype(originalFunc))HookshotInterface()->GetOriginalFunction(originalFunc))());
  }
} // namespace HookshotTest
