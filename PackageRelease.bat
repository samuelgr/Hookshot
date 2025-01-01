@echo off
setlocal enabledelayedexpansion

set project_name=Hookshot
set project_platforms=Win32 x64

set project_has_sdk=yes
set project_has_third_party_license=yes

set files_release=LICENSE README.md

set files_release_build_Win32=Hookshot.32.exe Hookshot.32.dll HookshotLauncher.32.exe
set files_release_build_x64=Hookshot.64.exe Hookshot.64.dll HookshotLauncher.64.exe


set files_sdk_lib_build_Win32=Hookshot.32.lib
set files_sdk_lib_build_x64=Hookshot.64.lib
set files_sdk_include=Include\Hookshot\*.h

set third_party_license=IntelXED

call Modules\Infra\Build\Scripts\PackageRelease.bat
