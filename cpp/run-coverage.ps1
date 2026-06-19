Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Command,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]] $Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE"
    }
}

function Test-CompilerSupportsStdMutex {
    param(
        [Parameter(Mandatory = $true)]
        [string] $CompilerPath
    )

    $probeDir = Join-Path $env:TEMP "rvc-coverage-compiler-probe"
    New-Item -ItemType Directory -Force $probeDir | Out-Null
    $source = Join-Path $probeDir "probe.cpp"
    $exe = Join-Path $probeDir "probe.exe"

    @"
#include <condition_variable>
#include <mutex>
int main() {
    std::mutex m;
    std::condition_variable cv;
    (void)m;
    (void)cv;
    return 0;
}
"@ | Set-Content -Encoding ASCII $source

    if (Test-Path $exe) {
        Remove-Item -Force $exe
    }

    $compilerBin = Split-Path -Parent $CompilerPath
    $oldPath = $env:PATH
    $env:PATH = "$compilerBin;$oldPath"

    $log = Join-Path $probeDir "probe.log"
    if (Test-Path $log) {
        Remove-Item -Force $log
    }

    $compileCommand = "`"$CompilerPath`" -std=c++17 `"$source`" -o `"$exe`" > `"$log`" 2>&1"
    & cmd.exe /c $compileCommand
    $success = $LASTEXITCODE -eq 0
    $env:PATH = $oldPath

    if (-not $success) {
        Write-Host "Compiler probe failed for: $CompilerPath"
        if (Test-Path $log) {
            Get-Content $log | Select-Object -First 12 | ForEach-Object { Write-Host "  $_" }
        }
    }

    return $success
}

function Find-CoverageCompiler {
    $candidates = @(
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\clang64\bin\clang++.exe",
        "C:\msys64\mingw64\bin\g++.exe"
    )

    $pathGxx = Get-Command g++ -ErrorAction SilentlyContinue
    if ($null -ne $pathGxx) {
        $candidates += $pathGxx.Source
    }

    $pathClang = Get-Command clang++ -ErrorAction SilentlyContinue
    if ($null -ne $pathClang) {
        $candidates += $pathClang.Source
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if ((Test-Path $candidate) -and (Test-CompilerSupportsStdMutex $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw @"
No GCC/Clang compiler with std::mutex support was found.

Your current MinGW appears to be the older Win32-thread variant, which cannot build GoogleTest.
Install MSYS2 UCRT64 on Windows 11, then rerun this script:

  winget install MSYS2.MSYS2

After MSYS2 is installed, open "MSYS2 UCRT64" and run:

  pacman -Syu
  pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-lcov

This script will automatically use C:\msys64\ucrt64\bin\g++.exe when it exists.
"@
}

function Convert-ToMsysPath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path
    )

    $fullPath = (Resolve-Path $Path).Path
    if ($fullPath -match '^([A-Za-z]):\\(.*)$') {
        $drive = $Matches[1].ToLower()
        $rest = $Matches[2] -replace '\\', '/'
        return "/$drive/$rest"
    }

    return $fullPath -replace '\\', '/'
}

function Find-MsysBash {
    $candidates = @(
        "C:\msys64\usr\bin\bash.exe",
        "C:\msys64\msys2_shell.cmd"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $fromPath = Get-Command bash -ErrorAction SilentlyContinue
    if ($null -ne $fromPath) {
        return $fromPath.Source
    }

    return $null
}

$buildDir = "build-coverage"

if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
}

$coverageCompiler = Find-CoverageCompiler
Write-Host "Using coverage compiler: $coverageCompiler"
$coverageCompilerBin = Split-Path -Parent $coverageCompiler
$env:PATH = "$coverageCompilerBin;$env:PATH"

$cmakeArgs = @(
    "-S", ".",
    "-B", $buildDir,
    "-DRVC_ENABLE_COVERAGE=ON",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_CXX_COMPILER=$coverageCompiler"
)

$ninjaPath = Join-Path $coverageCompilerBin "ninja.exe"
$mingwMakePath = Join-Path $coverageCompilerBin "mingw32-make.exe"
$ninja = if (Test-Path $ninjaPath) { Get-Item $ninjaPath } else { Get-Command ninja -ErrorAction SilentlyContinue }
$mingwMake = if (Test-Path $mingwMakePath) { Get-Item $mingwMakePath } else { Get-Command mingw32-make -ErrorAction SilentlyContinue }
if ($null -ne $ninja) {
    $cmakeArgs = @("-G", "Ninja") + $cmakeArgs
} elseif ($null -ne $mingwMake) {
    $cmakeArgs = @("-G", "MinGW Makefiles") + $cmakeArgs
}

Invoke-Checked cmake @cmakeArgs
Invoke-Checked cmake --build $buildDir --target rvc_controller_tests
Invoke-Checked ctest --test-dir $buildDir --output-on-failure

$bash = Find-MsysBash
$lcovScript = Join-Path $PSScriptRoot "scripts/run-lcov-report.sh"

if ($null -eq $bash -or -not (Test-Path $lcovScript)) {
    Write-Host "MSYS2 bash or scripts/run-lcov-report.sh was not found, so coverage data was generated but no report was created."
    Write-Host "Install MSYS2 and lcov with:"
    Write-Host "  pacman -S --needed mingw-w64-ucrt-x86_64-lcov"
    exit 0
}

$unixRoot = Convert-ToMsysPath $PSScriptRoot
$unixCompilerBin = Convert-ToMsysPath $coverageCompilerBin
$lcovCommand = "export PATH='$unixCompilerBin':`$PATH && cd '$unixRoot' && bash scripts/run-lcov-report.sh '$buildDir' '.'"

& $bash -lc $lcovCommand
if ($LASTEXITCODE -ne 0) {
    throw "lcov/genhtml failed with exit code $LASTEXITCODE"
}

Write-Host "Coverage info: $PSScriptRoot\$buildDir\coverage\coverage.filtered.info"
Write-Host "Coverage HTML report: $PSScriptRoot\$buildDir\coverage\html\index.html"
