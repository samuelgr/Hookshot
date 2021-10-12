@echo off
setlocal enabledelayedexpansion

rem +--------------------------------------------------------------------------
rem | Hookshot
rem |   General-purpose library for injecting DLLs and hooking function calls.
rem +--------------------------------------------------------------------------
rem | Authored by Samuel Grossman
rem | Copyright (c) 2019-2021
rem +--------------------------------------------------------------------------
rem | GitVersionInfo.bat
rem |   Script for extracting version information from Git. Executed
rem |   automatically on build.
rem +--------------------------------------------------------------------------

set script_path=%~dp0
set output_dir=%~f1
set extra_version_tag=%2

if "%output_dir%"=="" (
    set output_dir=%script_path%\Output
)

set output_file=%output_dir%\GitVersionInfo.h

if not exist %output_dir% md %output_dir%
if exist %output_file% del /f %output_file%

git --version >NUL 2>NUL
if %ERRORLEVEL%==0 (
    for /f "usebackq tokens=1" %%D in (`git diff --shortstat`) do (
        if not "%%~D"=="" (
            set is_dirty=yes
        ) else (
            set is_dirty=no
        )
    )

    for /f "usebackq delims=v tokens=1" %%V in (`git tag --list v[0-9]*.[0-9]*.[0-9]* --merged HEAD`) do set merged_release_ver=%%~V
    if not "!merged_release_ver!"=="" (
        for /f "usebackq delims=.- tokens=1" %%V in (`echo !merged_release_ver!`) do set version_major=%%~V
        for /f "usebackq delims=.- tokens=2" %%V in (`echo !merged_release_ver!`) do set version_minor=%%~V
        for /f "usebackq delims=.- tokens=3" %%V in (`echo !merged_release_ver!`) do set version_patch=%%~V
        for /f "usebackq" %%V in (`git rev-list --count v!merged_release_ver!..HEAD`) do set version_commit_distance=%%~V
    ) else (
        set version_major=0
        set version_minor=0
        set version_patch=0
        set version_commit_distance=0
    )

    if "!version_commit_distance!"=="0" (
        set version_string=!version_major!.!version_minor!.!version_patch!
    ) else (
        for /f "usebackq tokens=1" %%V in (`git describe --abbrev^=8 --always`) do (
            set version_string=!version_major!.!version_minor!.!version_patch!-dev.!version_commit_distance!
            set version_build_metadata=%%~V
        )
    )
    
    if "!is_dirty!"=="yes" (
        if "%extra_version_tag%"=="" (
            set extra_version_tag=dirty
        ) else (
            set extra_version_tag=%extra_version_tag%.dirty
        )
    )
) else (
    echo Git is not installed. Unable to determine version information.
    set version_major=0
    set version_minor=0
    set version_patch=0
    set version_commit_distance=0
    set version_string=!version_major!.!version_minor!.!version_patch!+unknown
)

if "%extra_version_tag%"=="" (
    if not "%version_build_metadata%"=="" (
        set version_string=%version_string%+%version_build_metadata%
    )
) else (
    if "%version_build_metadata%"=="" (
        set version_string=%version_string%+%extra_version_tag%
    ) else (
        set version_string=%version_string%+%version_build_metadata%.%extra_version_tag%
    )
)

set define_version_major=#define GIT_VERSION_MAJOR %version_major%
set define_version_minor=#define GIT_VERSION_MINOR %version_minor%
set define_version_patch=#define GIT_VERSION_PATCH %version_patch%
set define_version_commit_distance=#define GIT_VERSION_COMMIT_DISTANCE %version_commit_distance%
set define_version_struct=#define GIT_VERSION_STRUCT %version_major%,%version_minor%,%version_patch%,%version_commit_distance%
set define_version_string=#define GIT_VERSION_STRING "%version_string%"

echo %define_version_major%
echo %define_version_minor%
echo %define_version_patch%
echo %define_version_commit_distance%
echo %define_version_struct%
echo %define_version_string%

echo %define_version_major%             >  %output_dir%\GitVersionInfo.h
echo %define_version_minor%             >> %output_dir%\GitVersionInfo.h
echo %define_version_patch%             >> %output_dir%\GitVersionInfo.h
echo %define_version_commit_distance%   >> %output_dir%\GitVersionInfo.h
echo %define_version_struct%            >> %output_dir%\GitVersionInfo.h
echo %define_version_string%            >> %output_dir%\GitVersionInfo.h