@echo off
REM Build script for winDozer
REM Compiles the project and optionally runs it

setlocal

REM Try to find g++ in PATH, or use known MSYS2 location
set GPP=g++
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    REM Try MSYS2 location
    if exist "C:\msys64\mingw64\bin\g++.exe" (
        set GPP=C:\msys64\mingw64\bin\g++.exe
        echo Using g++ from MSYS2: %GPP%
    ) else (
        echo ERROR: g++ not found in PATH or at C:\msys64\mingw64\bin\g++.exe
        echo Please ensure MinGW/MSYS2 is installed and g++ is available
        exit /b 1
    )
)

echo Building winDozer...
%GPP% ^
    .\src\main.cpp ^
    .\src\WinDozer.cpp ^
    .\src\WinDozerUtils.cpp ^
    .\src\WinDozerInput.cpp ^
    -Wall ^
    -Wextra ^
    -static-libgcc ^
    -static-libstdc++ ^
    -std=c++17 ^
    -I.\src ^
    -lpsapi ^
    -o winDozer.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo Build successful! Executable: winDozer.exe
echo.

echo To run: winDozer.exe [flags]
echo Example: winDozer.exe verbose

endlocal
