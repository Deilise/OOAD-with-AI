Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

New-Item -ItemType Directory -Force build | Out-Null

g++ -std=c++17 `
    -I include `
    examples/demo.cpp `
    src/AutomaticCleaningSession.cpp `
    src/NavigationAndEscapeCoordinator.cpp `
    src/ObstaclePerceptionContext.cpp `
    src/RvcSoftwareController.cpp `
    src/SurfaceCleaningController.cpp `
    -o build/demo.exe

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

./build/demo.exe
