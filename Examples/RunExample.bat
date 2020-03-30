@echo off

rem Read command-line arguments.
set hookshot_exe=%1
set test_program_exe=%2
set out_dir=%3
set project_name=%4

rem Create the Hookshot.ini configuration file.
echo HookModule = %project_name% > %out_dir%Hookshot.ini

rem Run the Hookshot executable and point it to the test program.
%hookshot_exe% %test_program_exe%
