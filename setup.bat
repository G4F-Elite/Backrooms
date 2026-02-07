@echo off
echo ========================================
echo Backrooms VHS Horror - Setup Script
echo ========================================
echo.

echo Creating directories...
if not exist "deps" mkdir deps
if not exist "deps\include" mkdir deps\include
if not exist "deps\include\GLFW" mkdir deps\include\GLFW
if not exist "deps\include\glad" mkdir deps\include\glad
if not exist "deps\include\KHR" mkdir deps\include\KHR
if not exist "deps\lib" mkdir deps\lib
if not exist "build" mkdir build

echo.
echo Downloading GLFW...
curl -L -o deps\glfw.zip "https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip"
if errorlevel 1 (
    echo Failed to download GLFW. Please download manually from https://www.glfw.org/download
    pause
    exit /b 1
)

echo Extracting GLFW...
powershell -command "Expand-Archive -Path 'deps\glfw.zip' -DestinationPath 'deps' -Force"

echo Copying GLFW files...
copy "deps\glfw-3.3.8.bin.WIN64\include\GLFW\*" "deps\include\GLFW\" >nul
copy "deps\glfw-3.3.8.bin.WIN64\lib-mingw-w64\libglfw3.a" "deps\lib\" >nul
copy "deps\glfw-3.3.8.bin.WIN64\lib-mingw-w64\libglfw3dll.a" "deps\lib\" >nul
copy "deps\glfw-3.3.8.bin.WIN64\lib-mingw-w64\glfw3.dll" "build\" >nul 2>nul

echo.
echo Setup complete!
echo.
echo To build the game, run: build.bat
echo.
pause