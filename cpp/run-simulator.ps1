Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

cmake -S . -B build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build build --target rvc_simulator
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (Test-Path ".\build\rvc_simulator.exe") {
    .\build\rvc_simulator.exe @args
} elseif (Test-Path ".\build\Debug\rvc_simulator.exe") {
    .\build\Debug\rvc_simulator.exe @args
} else {
    Write-Error "Could not find rvc_simulator.exe after build."
    exit 1
}
