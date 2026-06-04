Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

cmake -S . -B build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build build --target rvc_controller_tests
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$env:GTEST_COLOR = "1"

if (Test-Path ".\build\rvc_controller_tests.exe") {
    .\build\rvc_controller_tests.exe --gtest_color=yes @args
    exit $LASTEXITCODE
} elseif (Test-Path ".\build\Debug\rvc_controller_tests.exe") {
    .\build\Debug\rvc_controller_tests.exe --gtest_color=yes @args
    exit $LASTEXITCODE
} else {
    Write-Error "Could not find rvc_controller_tests.exe after build."
    exit 1
}
