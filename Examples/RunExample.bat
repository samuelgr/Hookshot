@echo off
rem +-----------------------------------------------------------------------------------------------
rem | Hookshot
rem |   General-purpose library for injecting DLLs and hooking function calls.
rem +-----------------------------------------------------------------------------------------------
rem | Authored by Samuel Grossman
rem | Copyright (c) 2019-2024
rem +-----------------------------------------------------------------------------------------------
rem | RunExample.bat
rem |   Script for running hook module examples. Invoked by Visual Studio when attempting to run
rem |   a hook module example project.
rem +-----------------------------------------------------------------------------------------------

set hookshot_exe=%~f1
set test_program_exe=%~f2
set out_dir=%~f3
set project_name=%4

echo HookModule = %project_name% > %out_dir%Hookshot.ini
echo > NUL 2> %test_program_exe%.hookshot

if exist %hookshot_exe% (
    %hookshot_exe% %test_program_exe%
) else (
    echo %hookshot_exe%
    echo File does not exist! Build Hookshot and try again.
    pause
    exit /b 1
)
