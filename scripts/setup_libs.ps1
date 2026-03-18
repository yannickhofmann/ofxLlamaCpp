$ErrorActionPreference = 'Stop'

# This script clones the required libraries for ofxLlamaCpp.
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$libsDir = Join-Path $scriptDir '..\libs'
$libsDir = [System.IO.Path]::GetFullPath($libsDir)
$externalWindowsDir = [System.IO.Path]::GetFullPath((Join-Path $scriptDir '..\..\ofxLlamaCpp_build\windows'))
$vcpkgDir = Join-Path $externalWindowsDir 'vcpkg-openssl'
$vcpkgInstalledDir = Join-Path $vcpkgDir 'vcpkg_installed'
$openSslConfig = Join-Path $vcpkgInstalledDir 'x64-windows\share\openssl\OpenSSLConfig.cmake'

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git was not found in PATH. Install Git for Windows and try again."
}

function Resolve-CMakePath {
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCommand) {
        return $cmakeCommand.Source
    }

    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\CMake\bin\cmake.exe'
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "cmake was not found in PATH or standard install locations."
}

if (-not (Test-Path -LiteralPath $libsDir)) {
    New-Item -ItemType Directory -Path $libsDir | Out-Null
}

if (-not (Test-Path -LiteralPath $externalWindowsDir)) {
    New-Item -ItemType Directory -Path $externalWindowsDir | Out-Null
}

$repos = @(
    @{ Url = 'https://github.com/google/minja'; Target = 'minja' },
    @{ Url = 'https://github.com/ggml-org/llama.cpp'; Target = 'llama.cpp' }
)

foreach ($repo in $repos) {
    $targetDir = Join-Path $libsDir $repo.Target
    if (Test-Path -LiteralPath $targetDir) {
        Write-Host "Skipping $($repo.Target): already exists at $targetDir"
        continue
    }

    git clone $repo.Url $targetDir
}

[void](Resolve-CMakePath)

if (-not (Test-Path -LiteralPath $vcpkgDir)) {
    git clone https://github.com/microsoft/vcpkg $vcpkgDir
} else {
    Write-Host "Skipping external vcpkg-openssl: already exists at $vcpkgDir"
}

$vcpkgExe = Join-Path $vcpkgDir 'vcpkg.exe'
if (-not (Test-Path -LiteralPath $vcpkgExe)) {
    Write-Host "Bootstrapping vcpkg..."
    & (Join-Path $vcpkgDir 'bootstrap-vcpkg.bat') -disableMetrics
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg bootstrap failed."
    }
}

if (-not (Test-Path -LiteralPath $openSslConfig)) {
    Write-Host "Installing OpenSSL via vcpkg..."
    & $vcpkgExe install openssl:x64-windows "--x-install-root=$vcpkgInstalledDir"
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg install openssl:x64-windows failed."
    }
} else {
    Write-Host "Skipping OpenSSL install: already available at $openSslConfig"
}

Write-Host "Libraries and Windows OpenSSL toolchain prepared successfully."
