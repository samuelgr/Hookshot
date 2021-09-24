@echo off
setlocal enabledelayedexpansion

rem +--------------------------------------------------------------------------
rem | Hookshot
rem |   General-purpose library for injecting DLLs and hooking function calls.
rem +--------------------------------------------------------------------------
rem | Authored by Samuel Grossman
rem | Copyright (c) 2019-2021
rem +--------------------------------------------------------------------------
rem | PackageRelease.bat
rem |   Script for packaging up a release. To be executed manually after
rem |   building the Release configuration for both Win32 and x64 platforms.
rem +--------------------------------------------------------------------------

set project_name=Hookshot
set project_platforms=Win32 x64

set project_has_sdk=yes
set project_has_third_party_license=yes

set files_release=LICENSE *.md Output\Release\Hookshot.32.exe Output\Release\Hookshot.64.exe Output\Release\Hookshot.32.dll Output\Release\Hookshot.64.dll Output\Release\HookshotLauncher.32.exe Output\Release\HookshotLauncher.64.exe

set files_sdk_lib=Output\Release\Hookshot.32.lib Output\Release\Hookshot.64.lib
set files_sdk_include=Include\Hookshot\*.h

set third_party_license=IntelXED

rem ---------------------------------------------------------------------------

set script_path=%~dp0
set output_dir=%~f1

for /f "usebackq tokens=3" %%V in (`findstr VERSION_STRING %script_path%Resources\Version.h`) do set release_ver=%%~V
if "%release_ver%"=="" (
    echo Missing release version!
    goto :exit
)

if "%output_dir%"=="" (
    for /f "usebackq tokens=3*" %%D in (`reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders" /v Desktop`) do if "%%~E"=="" (set output_dir=%%~D) else (set output_dir=%%~D %%~E)
    call set output_dir=!output_dir!
)
if "%output_dir%"=="" (
    echo Missing output directory!
    goto :exit
)

set output_dir=%output_dir%\%project_name%-v%release_ver%

echo Releasing %project_name% v%release_ver% to %output_dir%.
choice /M "Proceed?"
if not %ERRORLEVEL%==1 exit /b

if exist %output_dir% (
    echo Output directory exists and will be overwritten.
    choice /M "Still proceed?"
    if not !ERRORLEVEL!==1 exit /b
    rd /S /Q %output_dir%
)

pushd %script_dir%
set files_are_missing=no
for %%F in (%files_release% %files_sdk_lib% %files_sdk_include%) do (
    if not exist %%F (
        echo Missing file: %%F
        set files_are_missing=yes
    )
)
for %%P in (%project_platforms%) do (
    if not ""=="%files_release_build%!files_release_build_%%P!" (
		for %%F in (%files_release_build% !files_release_build_%%P!) do (
            if not exist Output\%%P\Release\%%F (
                echo Missing file: Output\%%P\Release\%%F
                set files_are_missing=yes
            )
        )
    )
)
if "yes"=="%project_has_third_party_license%" (
    for %%T in (%third_party_license%) do (
        if not exist ThirdParty\%%T\LICENSE (
            echo Missing file: ThirdParty\%%T\LICENSE
            set files_are_missing=yes
        )
    )
)
popd

if "yes"=="%files_are_missing%" goto :exit

pushd %script_dir%
md %output_dir%
for %%F in (%files_release%) do (
    echo %%F
    copy %%F %output_dir%
)
for %%P in (%project_platforms%) do (
	if not ""=="%files_release_build%!files_release_build_%%P!" (
        md %output_dir%\%%P

        for %%F in (%files_release_build% !files_release_build_%%P!) do (
            echo Output\%%P\Release\%%F
            copy Output\%%P\Release\%%F %output_dir%\%%P
        )
    )
)
if "yes"=="%project_has_sdk%" (
    md %output_dir%\SDK

    if not ""=="%files_sdk_lib%" (
        md %output_dir%\SDK\Lib
        for %%F in (%files_sdk_lib%) do (
            echo %%F
            copy %%F %output_dir%\SDK\Lib
        )
    )

    if not ""=="%files_sdk_include%" (
        md %output_dir%\SDK\Include
        md %output_dir%\SDK\Include\%project_name%
        for %%F in (%files_sdk_include%) do (
            echo %%F
            copy %%F %output_dir%\SDK\Include\%project_name%
        )
    )
)
if "yes"=="%project_has_third_party_license%" (
    md %output_dir%\ThirdParty
    for %%T in (%third_party_license%) do (
        echo ThirdParty\%%T\LICENSE
        copy ThirdParty\%%T\LICENSE %output_dir%\ThirdParty\%%T_LICENSE
    )
)
popd

:exit
pause
