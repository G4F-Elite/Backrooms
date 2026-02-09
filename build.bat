@echo off
setlocal

echo ========================================
echo Backrooms VHS Horror - Build System
echo ========================================
echo.

cd /d "%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -File "tools\check_lines.ps1" -SrcPath "src" -MaxLines 500
if errorlevel 1 (
    echo.
    echo Build cancelled.
    exit /b 1
)

if not exist "build" mkdir build

echo.
echo Compiling...
g++ -std=c++17 -O2 -Wall -Wno-unused-result -Wno-unknown-pragmas -mwindows -static -static-libgcc -static-libstdc++ ^
    -I"deps/include" ^
    -o "build/backrooms.exe" ^
    "src/game.cpp" ^
    "deps/src/glad.c" ^
    -L"deps/lib" ^
    -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32

if errorlevel 1 (
    echo.
    echo Build FAILED!
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo Run: build\backrooms.exe
exit /b 0
