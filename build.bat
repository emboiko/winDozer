@echo off
REM Build script for winDozer
REM Compiles the project in debug or release mode
REM Usage: build.bat [debug|release]
REM Default: release

setlocal

REM Parse build mode argument
set BUILD_MODE=release
if "%1"=="debug" set BUILD_MODE=debug
if "%1"=="release" set BUILD_MODE=release

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

echo Building winDozer in %BUILD_MODE% mode...

tasklist /FI "IMAGENAME eq winDozer.exe" 2>NUL | find /I /N "winDozer.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo WARNING: winDozer.exe is currently running!
    echo Please close it before building, or the build will fail.
    echo.
    pause
)

REM Base compiler flags
set FLAGS=-Wall -Wextra -std=c++17 -I.\src

REM Add mode-specific flags
if "%BUILD_MODE%"=="debug" (
    set FLAGS=%FLAGS% -g -O0 -DDEBUG
    set OUTPUT=winDozer.exe
) else (
    set FLAGS=%FLAGS% -O2 -DNDEBUG -s
    set OUTPUT=winDozer.exe
)

REM Static linking flags for portability (prevents DLL dependencies)
REM -static: fully static link all MinGW libraries (libgcc, libstdc++, pthread, etc.)
REM This prevents libwinpthread-1.dll and other MinGW DLL dependencies
REM Windows system DLLs (kernel32, user32, etc.) remain dynamic as required
if "%BUILD_MODE%"=="release" (
    set LINK_FLAGS=-static -lpsapi
) else (
    REM Debug build: static link runtime libs but keep debug symbols
    set LINK_FLAGS=-static-libgcc -static-libstdc++ -Wl,-Bstatic -lpthread -Wl,-Bdynamic -lpsapi
)

%GPP% ^
    .\src\main.cpp ^
    .\src\WinDozer.cpp ^
    .\src\WinDozerUtils.cpp ^
    .\src\WinDozerInput.cpp ^
    %FLAGS% ^
    %LINK_FLAGS% ^
    -o %OUTPUT%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo Build successful! Executable: %OUTPUT%
echo Build mode: %BUILD_MODE%
echo.
echo To run: %OUTPUT% [flags]
echo Example: %OUTPUT% verbose

endlocal
