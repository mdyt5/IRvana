@echo off
setlocal enabledelayedexpansion

:: Use vswhere to get the latest Visual Studio installation path
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VCINSTALLDIR=%%i"
)

:: Get latest MSVC version (sorted, last one will be latest)
set "MSVC_VERSION="
for /f "delims=" %%i in ('dir /b /ad "%VCINSTALLDIR%\VC\Tools\MSVC" ^| sort') do (
    set "MSVC_VERSION=%%i"
)
set "VCTOOLSDIR=%VCINSTALLDIR%\VC\Tools\MSVC\%MSVC_VERSION%"

:: Get Windows SDK root from registry
for /f "tokens=3*" %%i in ('reg query "HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10 2^>nul') do (
    set "WINSDKDIR=%%i %%j"
)

:: Get latest SDK version (sorted, last one will be latest)
set "SDKVERSION="
for /f "delims=" %%i in ('dir /b /ad "!WINSDKDIR!\Include" ^| findstr "^10\." ^| sort') do (
    set "SDKVERSION=%%i"
)

:: Escape paths for Makefile (backslashes to double backslashes), remove quotes if any
set "WINSDKDIR_ESC=!WINSDKDIR:\=\\!"
set "VCTOOLSDIR_ESC=!VCTOOLSDIR:\=\\!"

:: Remove any surrounding quotes
set WINSDKDIR_ESC=!WINSDKDIR_ESC:"=!
set VCTOOLSDIR_ESC=!VCTOOLSDIR_ESC:"=!
set SDKVERSION=!SDKVERSION:"=!

:: Generate vs_env.mk without quotes
(
    echo winsdkdir := !WINSDKDIR_ESC!
    echo vctoolsdir := !VCTOOLSDIR_ESC!
    echo sdkver := !SDKVERSION!
) > vs_env.mk
