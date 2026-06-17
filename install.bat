@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  Flux Language Installer v1.0
echo ============================================
echo.

:: 1. 사용자/시스템 설치 선택창
echo Where do you want to add Flux to PATH?
echo [1] Current User only (No Admin required)
echo [2] All Users / System (Admin required)
echo.
set /p "CHOICE=Select an option (1 or 2): "

set "TARGET_SCOPE=User"
if "%CHOICE%"=="2" (
    set "TARGET_SCOPE=Machine"
    
    :: 관리자 권한 확인
    net session >nul 2>&1
    if !errorlevel! neq 0 (
        echo.
        echo [ERROR] Admin privileges are required for System-wide installation.
        echo         Please right-click this file and select 'Run as administrator'.
        echo.
        pause
        exit /b 1
    )
)

echo.
echo ============================================
:: Target directory: %LOCALAPPDATA%\Flux (Machine일 경우 Program Files 등으로 바꾸지 않고 기존 경로 유지)
set "TARGET=%LOCALAPPDATA%\Flux"
set "BIN_DIR=%TARGET%\bin"
set "MODULES_DIR=%TARGET%\modules"

echo Installing to: %TARGET%
echo Target PATH Scope: %TARGET_SCOPE%
echo.

:: Create directories
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%MODULES_DIR%" mkdir "%MODULES_DIR%"

:: Copy executables
echo Copying flux.exe...
copy /Y "%~dp0flux.exe" "%BIN_DIR%\flux.exe" >nul
echo Copying flux-pkg.exe...
copy /Y "%~dp0flux-pkg.exe" "%BIN_DIR%\flux-pkg.exe" >nul

:: Verify
if not exist "%BIN_DIR%\flux.exe" (
    echo Error: flux.exe not found. Run build.bat first.
    pause
    exit /b 1
)

echo.
echo Flux installed successfully!
echo.

:: [선택된 Scope에 따라 안전하게 PATH 추가]
echo Checking and updating %TARGET_SCOPE% PATH environment variable...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$scope = '%TARGET_SCOPE%'; ^
    $binDir = '%BIN_DIR%'; ^
    $currentPath = [Environment]::GetEnvironmentVariable('Path', $scope); ^
    if ($currentPath -split ';' -contains $binDir -or $currentPath -like '*Flux*') { ^
        Write-Host 'Already in' $scope 'PATH:' $binDir; ^
    } else { ^
        Write-Host 'Adding to' $scope 'PATH...'; ^
        $newPath = if ([string]::IsNullOrEmpty($currentPath)) { $binDir } else { $currentPath + ';' + $binDir }; ^
        [Environment]::SetEnvironmentVariable('Path', $newPath, $scope); ^
        Write-Host 'Successfully added Flux to' $scope 'PATH.'; ^
        Write-Host 'NOTE: You may need to restart your terminal for PATH changes.'; ^
    }"

echo.
echo ============================================
echo  Usage:
echo    flux script.fx       - Run a Flux script
echo    flux-pkg list         - List installed modules
echo    flux-pkg install math - Install math module
echo ============================================
echo.

pause
