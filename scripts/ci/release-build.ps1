$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $RootDir

$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { "build" }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Release" }
$BuildTesting = if ($env:BUILD_TESTING) { $env:BUILD_TESTING } else { "OFF" }

if (-not $env:RELEASE_PLATFORM) { $env:RELEASE_PLATFORM = "windows" }
if (-not $env:RELEASE_ARCH) { $env:RELEASE_ARCH = "x86_64" }

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [scriptblock]$Command,
        [string]$ErrorMessage = "Command failed"
    )

    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$ErrorMessage (exit code $LASTEXITCODE)"
    }
}

$ProfilePath = & (Join-Path $PSScriptRoot "release-conan-profile.ps1")
$env:CONAN_PROFILE = $ProfilePath.Trim()

Write-Host "Using Conan profile at $env:CONAN_PROFILE"
Get-Content $env:CONAN_PROFILE

Invoke-Checked {
    conan install . `
        --output-folder=$BuildDir `
        --build=missing `
        --lockfile-partial `
        -o "&:build_testing=False" `
        -s "build_type=$BuildType" `
        -pr $env:CONAN_PROFILE `
        -pr:b $env:CONAN_PROFILE
} -ErrorMessage "conan install failed"

Invoke-Checked {
    cmake --preset conan-release `
        -DBUILD_TESTING=$BuildTesting `
        -DBUILD_FUZZER=OFF `
        -DBUILD_COVERAGE=OFF `
        -DBUILD_CACHE=ON
} -ErrorMessage "cmake configure failed"

Invoke-Checked {
    cmake --build --preset conan-release --target beez --parallel
} -ErrorMessage "cmake build failed"

$BinaryPath = Join-Path $BuildDir "build/$BuildType/bin/beez.exe"
if (-not (Test-Path $BinaryPath)) {
    $discovered = Get-ChildItem -Path $BuildDir -Filter "beez.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($discovered) {
        $BinaryPath = $discovered
    }
    else {
        throw "release binary not found at $BinaryPath"
    }
}

Write-Host "Built $BinaryPath"
