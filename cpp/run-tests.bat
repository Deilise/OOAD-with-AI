@echo off
setlocal

cd /d "%~dp0"

cmake -S . -B build
if errorlevel 1 exit /b %errorlevel%

cmake --build build --target rvc_controller_tests
if errorlevel 1 exit /b %errorlevel%

set GTEST_COLOR=1

if exist build\rvc_controller_tests.exe (
    build\rvc_controller_tests.exe --gtest_color=yes %*
    exit /b %errorlevel%
)

if exist build\Debug\rvc_controller_tests.exe (
    build\Debug\rvc_controller_tests.exe --gtest_color=yes %*
    exit /b %errorlevel%
)

echo Could not find rvc_controller_tests.exe after build.
exit /b 1
