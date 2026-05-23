param(
    [switch]$EnableCuda,
    [switch]$EnableVulkan,
    [string]$BuildDir = '',
    [string]$Generator = 'Visual Studio 17 2022',
    [string]$Platform = 'x64',
    [string]$Toolset = 'v143',
    [Alias('Configuration')]
    [string[]]$Configurations = @('Debug', 'Release'),
    [switch]$Clean,
    [switch]$PurgeBuild,
    [switch]$PurgeAll
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

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

function Resolve-GitPath {
    $gitCommand = Get-Command git.exe -ErrorAction SilentlyContinue
    if ($gitCommand) {
        return $gitCommand.Source
    }

    $candidates = @(
        'C:\Users\ma_YH\AppData\Local\vcpkg\downloads\tools\git-2.7.4-windows\cmd\git.exe',
        'C:\Users\ma_YH\AppData\Local\vcpkg\downloads\tools\git-2.7.4-windows\bin\git.exe',
        'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd\git.exe',
        'C:\Program Files\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd\git.exe',
        'C:\Program Files\Git\cmd\git.exe',
        'C:\Program Files\Git\bin\git.exe'
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    return $null
}

function Copy-LibraryIfPresent {
    param(
        [string]$BuildDir,
        [string]$Configuration,
        [string]$FileName,
        [string]$DestinationDir,
        [bool]$Required = $false
    )

    $matches = Get-ChildItem -Path $BuildDir -Recurse -Filter $FileName -File

    $match = $matches |
        Where-Object { ($_.FullName -split '\\') -contains $Configuration } |
        Select-Object -First 1

    if (-not $match) {
        $match = $matches | Select-Object -First 1
    }

    if (-not $match) {
        if ($Required) {
            throw "$FileName not found for $Configuration in $BuildDir"
        }

        Write-Host "Skipping optional $FileName"
        return
    }

    Copy-Item -LiteralPath $match.FullName -Destination $DestinationDir -Force
    Write-Host "Copied $FileName"
}

function Copy-LibrariesForConfiguration {
    param(
        [string]$BuildDir,
        [string]$Configuration,
        [string]$DestinationRoot,
        [bool]$CudaEnabled
    )

    $destinationDir = Join-Path $DestinationRoot $Configuration

    if (-not (Test-Path -LiteralPath $destinationDir)) {
        New-Item -ItemType Directory -Force -Path $destinationDir | Out-Null
    }

    $libraries = @(
        'llama.lib',
        'ggml.lib',
        'ggml-cpu.lib',
        'ggml-base.lib',
        'mtmd.lib'
    )

    $optionalLibraries = @(
        'llama-common.lib',
        'llama-common-base.lib',
        'common.lib',
        'cpp-httplib.lib',
        'ggml-vulkan.lib',
        'vulkan.lib'
    )

    if ($CudaEnabled) {
        $libraries += 'ggml-cuda.lib'
    }

    foreach ($library in $libraries) {
        Copy-LibraryIfPresent -BuildDir $BuildDir -Configuration $Configuration -FileName $library -DestinationDir $destinationDir -Required $true
    }

    foreach ($library in $optionalLibraries) {
        Copy-LibraryIfPresent -BuildDir $BuildDir -Configuration $Configuration -FileName $library -DestinationDir $destinationDir -Required $false
    }
}

function Remove-ManagedRootLibraries {
    param([string]$DestinationRoot)

    $libs = @(
        'llama.lib',
        'ggml.lib',
        'ggml-cpu.lib',
        'ggml-base.lib',
        'mtmd.lib',
        'llama-common.lib',
        'llama-common-base.lib',
        'common.lib',
        'cpp-httplib.lib',
        'ggml-vulkan.lib',
        'vulkan.lib',
        'ggml-cuda.lib'
    )

    foreach ($lib in $libs) {
        $matches = Get-ChildItem -Path $DestinationRoot -Recurse -Filter $lib -File -ErrorAction SilentlyContinue
        foreach ($match in $matches) {
            Remove-Item -LiteralPath $match.FullName -Force
        }
    }
}

Write-Host "========================================="
Write-Host " Windows llama.cpp Static Build Script"
Write-Host "========================================="

$cmakeExe = Resolve-CMakePath
$gitExe = Resolve-GitPath
$addonDir = Join-Path $scriptDir '..'
$llamaDir = Join-Path $addonDir 'libs\llama.cpp'
$destDir = Join-Path $llamaDir 'lib\vs'
$externalWindowsRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDir '..\..\ofxLlamaCpp_build\windows'))

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $buildDir = Join-Path $externalWindowsRoot 'build'
} elseif ([System.IO.Path]::IsPathRooted($BuildDir)) {
    $buildDir = [System.IO.Path]::GetFullPath($BuildDir)
} else {
    $buildDir = [System.IO.Path]::GetFullPath((Join-Path $scriptDir $BuildDir))
}

if (-not (Test-Path -LiteralPath $llamaDir)) {
    throw "llama.cpp was not found at $llamaDir. Run setup_libs.ps1 first."
}

$autoEnableCuda = $EnableCuda.IsPresent -or (
    (Get-Command nvcc -ErrorAction SilentlyContinue) -and
    (Get-Command nvidia-smi -ErrorAction SilentlyContinue) -and
    ((& nvidia-smi -L) -ne $null)
)

if ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Write-Host "Cleaning previous build directory"
    try {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    } catch {
        Write-Warning "Could not fully remove $buildDir. Continuing with the existing build directory."
    }
}

$cmakeArgs = @(
    '-S', $llamaDir,
    '-B', $buildDir,
    '-G', $Generator,
    '-A', $Platform
)

if ($Generator -like 'Visual Studio*' -and -not [string]::IsNullOrWhiteSpace($Toolset)) {
    $cmakeArgs += '-T'
    $cmakeArgs += $Toolset
}

$cmakeArgs += @(
    '-DBUILD_SHARED_LIBS=OFF',
    '-DGGML_STATIC=ON',
    '-DLLAMA_BUILD_COMMON=ON',
    '-DLLAMA_BUILD_TOOLS=ON',
    '-DLLAMA_BUILD_TESTS=OFF',
    '-DLLAMA_BUILD_EXAMPLES=OFF',
    '-DLLAMA_BUILD_SERVER=OFF',
    '-DLLAMA_CURL=OFF',
    '-DLLAMA_OPENSSL=OFF',
    '-DCMAKE_CXX_STANDARD=17',
    '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL',
    '-DGGML_NATIVE=ON',
    '-DGGML_OPENMP=OFF',
    '-DGGML_LTO=OFF',
    '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF'
)

if ($gitExe) {
    $cmakeArgs += "-DGIT_EXECUTABLE=$gitExe"
}

if ($autoEnableCuda) {
    Write-Host "CUDA enabled"
    $cmakeArgs += '-DGGML_CUDA=ON'
} else {
    Write-Host "CPU-only build"
}

if ($EnableVulkan) {
    Write-Host "Vulkan enabled"
    $cmakeArgs += '-DGGML_VULKAN=ON'
}

Write-Host "Destination directory: $destDir"
Write-Host "Configurations: $($Configurations -join ', ')"
if ($Generator -like 'Visual Studio*' -and -not [string]::IsNullOrWhiteSpace($Toolset)) {
    Write-Host "MSVC toolset: $Toolset"
}
Write-Host "OpenSSL: disabled for the static addon build"

Write-Host "Configuring CMake..."
& $cmakeExe @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed"
}

foreach ($configuration in $Configurations) {
    Write-Host "Building $configuration"
    $targets = @('llama', 'ggml', 'ggml-cpu', 'ggml-base', 'mtmd')
    if ($autoEnableCuda) {
        $targets += 'ggml-cuda'
    }
    if ($EnableVulkan) {
        $targets += 'ggml-vulkan'
    }

    foreach ($target in $targets) {
        Write-Host "  target: $target"
        & $cmakeExe --build $buildDir --config $configuration --target $target --parallel
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed for $configuration target $target"
        }
    }
}

if (-not (Test-Path -LiteralPath $destDir)) {
    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
}

Remove-ManagedRootLibraries -DestinationRoot $destDir

foreach ($configuration in $Configurations) {
    Copy-LibrariesForConfiguration -BuildDir $buildDir -Configuration $configuration -DestinationRoot $destDir -CudaEnabled $autoEnableCuda
}

if ($PurgeBuild -and (Test-Path -LiteralPath $buildDir)) {
    Write-Host "Purging external build directory: $buildDir"
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}

if ($PurgeAll -and (Test-Path -LiteralPath $externalWindowsRoot)) {
    Write-Host "Purging external workspace: $externalWindowsRoot"
    Remove-Item -LiteralPath $externalWindowsRoot -Recurse -Force
}

Write-Host "========================================="
Write-Host "llama.cpp build complete"
Write-Host "Libraries in: $destDir\<Configuration>"
Write-Host "========================================="
