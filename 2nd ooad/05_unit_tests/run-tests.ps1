Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

& ..\..\cpp\run-tests.ps1 @args
exit $LASTEXITCODE
