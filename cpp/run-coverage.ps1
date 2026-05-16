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
  pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja

This script will automatically use C:\msys64\ucrt64\bin\g++.exe when it exists.
"@
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

$gcovr = Get-Command gcovr -ErrorAction SilentlyContinue
if ($null -eq $gcovr) {
    Write-Host "gcovr is not installed, so coverage data was generated but no report was created."
    Write-Host "Install it with: python -m pip install gcovr"
    exit 0
}

$coverageDir = Join-Path $buildDir "coverage"
New-Item -ItemType Directory -Force $coverageDir | Out-Null

$gcovrTextArgs = @("--root", ".", "--object-directory", $buildDir, "--filter", "src", "--filter", "include", "--exclude", "tests", "--txt")
$gcovrHtmlArgs = @("--root", ".", "--object-directory", $buildDir, "--filter", "src", "--filter", "include", "--exclude", "tests", "--html", "--html-details", "-o", "$coverageDir/coverage.html")
$gcovrXmlArgs = @("--root", ".", "--object-directory", $buildDir, "--filter", "src", "--filter", "include", "--exclude", "tests", "--xml", "-o", "$coverageDir/coverage.xml")

Invoke-Checked gcovr @gcovrTextArgs
Invoke-Checked gcovr @gcovrHtmlArgs
Invoke-Checked gcovr @gcovrXmlArgs

Write-Host "Coverage HTML report: $PSScriptRoot\$coverageDir\coverage.html"
