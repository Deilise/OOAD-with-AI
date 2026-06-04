Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

cmake -S ..\..\cpp -B ..\..\cpp\build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build ..\..\cpp\build --target rvc_simulator
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (Test-Path "..\..\cpp\build\rvc_simulator.exe") {
    ..\..\cpp\build\rvc_simulator.exe @args
    exit $LASTEXITCODE
} elseif (Test-Path "..\..\cpp\build\Debug\rvc_simulator.exe") {
    ..\..\cpp\build\Debug\rvc_simulator.exe @args
    exit $LASTEXITCODE
} else {
    Write-Error "Could not find rvc_simulator.exe after build."
    exit 1
}
