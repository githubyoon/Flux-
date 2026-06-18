@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  Flux Language Installer v1.0
echo ============================================
echo.

:: Visual Studio 환경 로드
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Visual Studio Build Tools not found.
    pause
    exit /b 1
)

:: =========================
:: Build
:: =========================
echo Building Flux...

cl /std:c++20 /utf-8 /EHsc /Iinclude ^
    src/main.cpp ^
    src/Lexer.cpp ^
    src/Parser.cpp ^
    src/Compiler.cpp ^
    src/VM.cpp ^
    /Feflux.exe >nul

if errorlevel 1 (
    echo [ERROR] Failed to build flux.exe
    pause
    exit /b 1
)

cl /std:c++20 /utf-8 /EHsc /Iinclude ^
    flux-pkg/main.cpp ^
    /Feflux-pkg.exe ^
    /link urlmon.lib winhttp.lib >nul

if errorlevel 1 (
    echo [ERROR] Failed to build flux-pkg.exe
    pause
    exit /b 1
)

echo Build completed.
echo.

:: =========================
:: Install scope
:: =========================
echo Where do you want to add Flux to PATH?
echo [1] Current User only
echo [2] All Users / System
echo.

set /p "CHOICE=Select an option (1 or 2): "

set "TARGET_SCOPE=User"

if "%CHOICE%"=="2" (
    set "TARGET_SCOPE=Machine"

    net session >nul 2>&1
    if !errorlevel! neq 0 (
        echo.
        echo [ERROR] Administrator privileges are required.
        pause
        exit /b 1
    )
)

:: =========================
:: Install
:: =========================
set "TARGET=%LOCALAPPDATA%\Flux"
set "BIN_DIR=%TARGET%\bin"
set "MODULES_DIR=%TARGET%\modules"

echo.
echo Installing to:
echo %TARGET%
echo.

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%MODULES_DIR%" mkdir "%MODULES_DIR%"

copy /Y flux.exe "%BIN_DIR%\flux.exe" >nul
copy /Y flux-pkg.exe "%BIN_DIR%\flux-pkg.exe" >nul

if not exist "%BIN_DIR%\flux.exe" (
    echo [ERROR] Installation failed.
    pause
    exit /b 1
)

:: =========================
:: PATH
:: =========================
echo Updating PATH...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
"$scope='%TARGET_SCOPE%';" ^
"$bin='%BIN_DIR%';" ^
"$path=[Environment]::GetEnvironmentVariable('Path',$scope);" ^
"if(($path -split ';') -contains $bin){" ^
"Write-Host 'Already in PATH.';" ^
"} else {" ^
"[Environment]::SetEnvironmentVariable('Path',($path+';'+$bin),$scope);" ^
"Write-Host 'Added to PATH.';" ^
"}"

echo.
echo ============================================
echo  Installation completed!
echo.
echo  Usage:
echo    flux script.fx
echo    flux-pkg list
echo    flux-pkg install math
echo.
echo  Restart your terminal to apply PATH changes.
echo ============================================
echo.

pause