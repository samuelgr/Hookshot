#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <hookshot.h>


#define HOOKSHOT_TEST_FUNCTION              __declspec(noinline)


HOOKSHOT_TEST_FUNCTION int TestFunction(int x)
{
    return (x * 2) + 55;
}


HOOKSHOT_TEST_FUNCTION int HookTestFunction(int x)
{
    return (x * 10) + 88;
}


int Pass(void)
{
    puts("PASS\n");
    return 0;
}


int Fail(int code)
{
    puts("FAIL\n");
    return code;
}


int main(int argc, const char* argv[])
{
    Hookshot::IHookConfig* hookConfig = HookshotLibraryInitialize();

    if (NULL == hookConfig)
        return Fail(1);

    if (255 != TestFunction(100))
        return Fail(2);

    const Hookshot::THookID testHookID = hookConfig->SetHook((void*)&HookTestFunction, (void*)&TestFunction);
    if (!Hookshot::SuccessfulResult(testHookID))
        return Fail(3);

    if (2088 != TestFunction(200))
        return Fail(4);

    const auto originalFunc = (decltype(&TestFunction))hookConfig->GetOriginalFunctionForHook(testHookID);
    if (NULL == originalFunc)
        return Fail(5);

    if (655 != originalFunc(300))
        return Fail(6);

    return Pass();
}
