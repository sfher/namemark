@echo off
setlocal enabledelayedexpansion

:: ============================================================
::  Namemark MSVC Build Script
::  Usage:
::    build.bat          Release build (default)
::    build.bat debug    Debug build
::    build.bat clean    Remove build artifacts
::    build.bat rebuild  Clean + Release build
:: ============================================================

set "TARGET=namemark.exe"
set "SRCDIR=src"
set "LIBDIR=lib"

:: --- Parse command line ---
set "BUILD_MODE=release"
set "DO_CLEAN=0"
if /i "%~1"=="clean"   (call :clean & goto :eof)
if /i "%~1"=="debug"   set "BUILD_MODE=debug"
if /i "%~1"=="rebuild" set "DO_CLEAN=1"

:: --- Locate MSVC Build Tools ---
call :find_msvc
if "%MSVC_PATH%"=="" (
    echo [ERROR] Cannot find Visual Studio Build Tools.
    echo Please install Visual Studio Build Tools 2022+ from:
    echo   https://visualstudio.microsoft.com/downloads/
    echo.
    echo Or run from a "Developer Command Prompt for VS" where MSVC
    echo environment is already configured.
    exit /b 1
)
echo MSVC: %MSVC_PATH%
echo WINSDK: %WINSDK_PATH%

:: --- Setup environment ---
call :setup_env

:: --- Clean if requested ---
if "%DO_CLEAN%"=="1" call :clean

:: --- Compile flags ---
set "CXXFLAGS=/std:c++20 /EHsc /Isrc /Ilib /nologo /utf-8 /D_CRT_SECURE_NO_WARNINGS"
if /i "%BUILD_MODE%"=="debug" (
    set "CXXFLAGS=%CXXFLAGS% /Zi /Od /DDEBUG"
    set "LDFLAGS=/DEBUG"
) else (
    set "CXXFLAGS=%CXXFLAGS% /O2 /DNDEBUG"
    set "LDFLAGS="
)

:: --- Source file list ---
set "SRCS="
for %%f in (act console customio damage_calculator entity file_utils game generic_skill level_data main monster_preset package_manager selection_list skill_data skill_executor skill_loader weapon_data weapon_loader) do (
    set "SRCS=!SRCS! %SRCDIR%\%%f.cpp"
)
for %%f in (adventure_state gacha_state lobby_state setting_state shop_state team_state teamtest_state) do (
    set "SRCS=!SRCS! %SRCDIR%\states\%%f.cpp"
)
set "SRCS=!SRCS! %LIBDIR%\customio23.cpp"

:: --- Compile ---
set "OBJS="
set "FAIL=0"
for %%s in (%SRCS%) do (
    set "OBJ=%%~ns.obj"
    set "OBJS=!OBJS! !OBJ!"
    echo [Compile] %%~nxs
    cl %CXXFLAGS% /c "%%s" /Fo:"!OBJ!" /Fd:namemark.pdb
    if errorlevel 1 (
        set "FAIL=1"
        echo [ERROR] Failed to compile %%s
        goto :link
    )
)

:: --- Link ---
:link
if "%FAIL%"=="1" (
    echo.
    echo Build FAILED with compilation errors.
    exit /b 1
)

echo.
echo [Link] %TARGET%
link /nologo %LDFLAGS% /OUT:%TARGET% %OBJS%
if errorlevel 1 (
    echo [ERROR] Linking failed.
    exit /b 1
)

echo.
echo ============================================
echo   Build SUCCESS: %TARGET% [%BUILD_MODE%]
echo ============================================
exit /b 0


:: ============================================================
::  Subroutines
:: ============================================================

:find_msvc
    :: Prefer vcvars if already in a VS command prompt
    if defined VCINSTALLDIR (
        set "VSROOT=%VCINSTALLDIR:~0,-3%"
        goto :find_msvc_ver
    )

    :: Try vswhere.exe (bundled with VS 2017+)
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
            if not "%%i"=="" set "VSROOT=%%i"
        )
    )

    :: Fallback: search common paths
    if not defined VSROOT (
        for %%d in (
            "C:\Program Files\Microsoft Visual Studio\2022"
            "C:\Program Files (x86)\Microsoft Visual Studio\2022"
            "C:\Program Files\Microsoft Visual Studio\18"
            "C:\Program Files (x86)\Microsoft Visual Studio\18"
        ) do (
            for %%e in (BuildTools Community Professional Enterprise) do (
                if exist "%%~d\%%e\VC\Auxiliary\Build\vcvars64.bat" (
                    set "VSROOT=%%~d\%%e"
                    goto :find_msvc_ver
                )
            )
        )
    )

    :: Fallback: wildcard search
    if not defined VSROOT (
        for /d %%d in ("C:\Program Files (x86)\Microsoft Visual Studio\*") do (
            for %%e in (BuildTools Community Professional Enterprise) do (
                if exist "%%d\%%e\VC\Auxiliary\Build\vcvars64.bat" (
                    set "VSROOT=%%d\%%e"
                    goto :find_msvc_ver
                )
            )
        )
    )

    if not defined VSROOT goto :eof

:find_msvc_ver
    :: Find latest MSVC toolchain version (dir /o-n = reverse name order)
    for /f "tokens=*" %%i in ('dir /b /ad /o-n "!VSROOT!\VC\Tools\MSVC" 2^>nul') do (
        set "MSVC_VER=%%i"
        goto :found_msvc
    )
    :found_msvc
    if not defined MSVC_VER goto :eof

    set "MSVC_PATH=!VSROOT!\VC\Tools\MSVC\!MSVC_VER!"

    :: Find latest Windows SDK
    for /f "tokens=*" %%i in ('dir /b /ad /o-n "C:\Program Files (x86)\Windows Kits\10\Lib" 2^>nul') do (
        set "WINSDK_VER=%%i"
        goto :found_sdk
    )
    :found_sdk
    set "WINSDK_PATH=C:\Program Files (x86)\Windows Kits\10"

    goto :eof


:setup_env
    set "PATH=!MSVC_PATH!\bin\Hostx64\x64;!PATH!"
    set "INCLUDE=!MSVC_PATH!\include;!WINSDK_PATH!\Include\!WINSDK_VER!\ucrt;!WINSDK_PATH!\Include\!WINSDK_VER!\shared;!WINSDK_PATH!\Include\!WINSDK_VER!\um;!WINSDK_PATH!\Include\!WINSDK_VER!\winrt;!WINSDK_PATH!\Include\!WINSDK_VER!\cppwinrt"
    set "LIB=!MSVC_PATH!\lib\x64;!WINSDK_PATH!\Lib\!WINSDK_VER!\ucrt\x64;!WINSDK_PATH!\Lib\!WINSDK_VER!\um\x64"
    goto :eof


:clean
    echo [Clean] Removing build artifacts...
    del /q *.obj 2>nul
    del /q *.pdb 2>nul
    del /q *.ilk 2>nul
    del /q *.exp 2>nul
    del /q *.lib 2>nul
    del /q %TARGET% 2>nul
    echo [Clean] Done.
    goto :eof
