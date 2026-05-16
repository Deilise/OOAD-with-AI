@echo off
setlocal

cd /d "%~dp0"

cmake -S . -B build
if errorlevel 1 exit /b %errorlevel%

cmake --build build --target rvc_simulator
if errorlevel 1 exit /b %errorlevel%

if exist build\rvc_simulator.exe (
    build\rvc_simulator.exe %*
    exit /b %errorlevel%
)

if exist build\Debug\rvc_simulator.exe (
    build\Debug\rvc_simulator.exe %*
    exit /b %errorlevel%
)

echo Could not find rvc_simulator.exe after build.
exit /b 1
