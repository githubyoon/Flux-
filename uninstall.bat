@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  Flux Language Uninstaller v1.0 (Auto Clean)
echo ============================================
echo.

:: 1. 관리자 권한 강제 확인
net session >nul 2>&1
if !errorlevel! neq 0 (
    echo [ERROR] This script must be run as Administrator.
    echo         Please right-click this file and select 'Run as administrator'.
    echo.
    pause
    exit /b 1
)

:: Target directory
set "TARGET=%LOCALAPPDATA%\Flux"

echo [PROCESS] Uninstalling Flux from %TARGET%...
echo.

:: 2. Flux 디렉터리(폴더) 삭제
if exist "%TARGET%" (
    rmdir /s /q "%TARGET%"
    echo [OK] Removed directory %TARGET%
) else (
    echo [INFO] Flux directory was not found.
)

:: 3. User 및 Machine PATH 양쪽 모두에서 Flux 제거
echo [PROCESS] Cleaning up PATH environment variables...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "foreach ($scope in @('User', 'Machine')) { ^
        $currentPath = [Environment]::GetEnvironmentVariable('Path', $scope); ^
        if ($currentPath -like '*Flux*') { ^
            $newPath = ($currentPath -split ';' | Where-Object { $_ -notlike '*Flux*' }) -join ';'; ^
            [Environment]::SetEnvironmentVariable('Path', $newPath, $scope); ^
            Write-Host ('   -> Successfully removed Flux from ' + $scope + ' PATH.'); ^
        } else { ^
            Write-Host ('   -> Flux was not found in ' + $scope + ' PATH.'); ^
        } ^
    }"

echo.
echo Done. All traces of Flux have been removed.
pause
