@echo off
setlocal

cd /d "%~dp0"

set BUILD_DIR=build-coverage

if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%

set COVERAGE_CXX=
if exist C:\msys64\ucrt64\bin\g++.exe set COVERAGE_CXX=C:\msys64\ucrt64\bin\g++.exe
if "%COVERAGE_CXX%"=="" if exist C:\msys64\clang64\bin\clang++.exe set COVERAGE_CXX=C:\msys64\clang64\bin\clang++.exe
if "%COVERAGE_CXX%"=="" if exist C:\msys64\mingw64\bin\g++.exe set COVERAGE_CXX=C:\msys64\mingw64\bin\g++.exe
if "%COVERAGE_CXX%"=="" (
    where g++ >nul 2>nul
    if not errorlevel 1 set COVERAGE_CXX=g++
)

if "%COVERAGE_CXX%"=="" (
    echo No GCC/Clang compiler was found.
    echo Install MSYS2 UCRT64 with:
    echo winget install MSYS2.MSYS2
    echo Then in MSYS2 UCRT64:
    echo pacman -Syu
    echo pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
    exit /b 1
)

echo Using coverage compiler: %COVERAGE_CXX%

for %%I in ("%COVERAGE_CXX%") do set COVERAGE_BIN=%%~dpI
set PATH=%COVERAGE_BIN%;%PATH%

set GENERATOR_ARGS=
if exist "%COVERAGE_BIN%ninja.exe" (
    set GENERATOR_ARGS=-G Ninja
) else (
    if exist "%COVERAGE_BIN%mingw32-make.exe" (
        set GENERATOR_ARGS=-G "MinGW Makefiles"
    ) else (
        where ninja >nul 2>nul
        if not errorlevel 1 (
            set GENERATOR_ARGS=-G Ninja
        ) else (
            where mingw32-make >nul 2>nul
            if not errorlevel 1 set GENERATOR_ARGS=-G "MinGW Makefiles"
        )
    )
)

cmake %GENERATOR_ARGS% -S . -B %BUILD_DIR% -DRVC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="%COVERAGE_CXX%"
if errorlevel 1 exit /b %errorlevel%

cmake --build %BUILD_DIR% --target rvc_controller_tests
if errorlevel 1 exit /b %errorlevel%

ctest --test-dir %BUILD_DIR% --output-on-failure
if errorlevel 1 exit /b %errorlevel%

where gcovr >nul 2>nul
if errorlevel 1 (
    echo gcovr is not installed, so coverage data was generated but no report was created.
    echo Install it with: python -m pip install gcovr
    exit /b 0
)

if not exist %BUILD_DIR%\coverage mkdir %BUILD_DIR%\coverage

gcovr --root . --object-directory %BUILD_DIR% --filter src --filter include --exclude tests --txt
gcovr --root . --object-directory %BUILD_DIR% --filter src --filter include --exclude tests --html --html-details -o %BUILD_DIR%\coverage\coverage.html
gcovr --root . --object-directory %BUILD_DIR% --filter src --filter include --exclude tests --xml -o %BUILD_DIR%\coverage\coverage.xml

echo Coverage HTML report: %CD%\%BUILD_DIR%\coverage\coverage.html
