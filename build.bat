@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Backrooms VHS Horror - Build System
echo ========================================
echo.
echo CODE STYLE REQUIREMENTS:
echo - Max 250 lines per file
echo - Each module has ONE responsibility
echo - Use readable formatting (not minified)
echo - Split large functions into helpers
echo ========================================
echo.

cd /d "%~dp0"

:: Check file sizes
echo Checking file sizes...
set "too_large=0"
for %%f in (src\*.cpp src\*.c src\*.h src\*.hpp) do (
    if exist "%%f" (
        set "lines=0"
        for /f %%l in ('type "%%f" ^| find /c /v ""') do set "lines=%%l"
        if !lines! gtr 250 ( :: DO NOT EDIT LIMIT!!! EDIT YOUR CODE INTO SMALLER FILES WITHOUT MINIFIING
            echo ERROR: %%f has !lines! lines - max 2500!
            set "too_large=1"
        )
    )
)

if !too_large! equ 1 (
    echo.
    echo ========================================
    echo BUILD REFUSED - Code too large!
    echo.
    echo Split your code into smaller modules:
    echo - net.h         = Networking only
    echo - player_model.h = Player rendering
    echo - menu_multi.h  = Multiplayer menus
    echo - audio.h       = Sound only
    echo - world.h       = World gen only
    echo ========================================
    exit /b 1
)
echo All files OK
echo.

if not exist "build" mkdir build

echo Compiling (Static Build)...
g++ -std=c++17 -O2 -Wall -Wno-unused-result -Wno-unknown-pragmas -mwindows -static -static-libgcc -static-libstdc++ ^
    -I"deps/include" ^
    -o "build/backrooms.exe" ^
    "src/game.cpp" ^
    "deps/src/glad.c" ^
    -L"deps/lib" ^
    -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32

if !errorlevel! neq 0 (
    echo.
    echo Build FAILED!
    exit /b 1
)

if exist "deps/lib/glfw3.dll" (
    copy /Y "deps/lib/glfw3.dll" "build\" >nul 2>&1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo.
echo Run: build\backrooms.exe
echo.
echo MULTIPLAYER:
echo - Host: Start game, go to Multiplayer ^> Host
echo - Join: Use Radmin/Hamachi IP, port 27015
echo.
