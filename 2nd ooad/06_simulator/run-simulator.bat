@echo off
setlocal

cd /d "%~dp0"

cmake -S ..\..\cpp -B ..\..\cpp\build
if errorlevel 1 exit /b %errorlevel%

cmake --build ..\..\cpp\build --target rvc_simulator
if errorlevel 1 exit /b %errorlevel%

if exist ..\..\cpp\build\rvc_simulator.exe (
    ..\..\cpp\build\rvc_simulator.exe %*
    exit /b %errorlevel%
)

if exist ..\..\cpp\build\Debug\rvc_simulator.exe (
    ..\..\cpp\build\Debug\rvc_simulator.exe %*
    exit /b %errorlevel%
)

echo Could not find rvc_simulator.exe after build.
exit /b 1
