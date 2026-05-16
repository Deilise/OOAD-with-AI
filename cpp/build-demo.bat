@echo off
setlocal

cd /d "%~dp0"

if not exist build mkdir build

g++ -std=c++17 ^
    -I include ^
    examples\demo.cpp ^
    src\AutomaticCleaningSession.cpp ^
    src\NavigationAndEscapeCoordinator.cpp ^
    src\ObstaclePerceptionContext.cpp ^
    src\RvcSoftwareController.cpp ^
    src\SurfaceCleaningController.cpp ^
    -o build\demo.exe

if errorlevel 1 exit /b %errorlevel%

build\demo.exe
