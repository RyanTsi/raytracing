@echo off
setlocal
cd /d "%~dp0"
cmake --preset windows-release
if errorlevel 1 exit /b 1
cmake --build --preset windows-release
if errorlevel 1 exit /b 1
ctest --preset windows-release
if errorlevel 1 exit /b 1
echo Run build\Release\rtrt.exe to start the renderer.
