$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $RootDir

$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { "build" }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Release" }
$BuildTesting = if ($env:BUILD_TESTING) { $env:BUILD_TESTING } else { "OFF" }

if (-not $env:RELEASE_PLATFORM) { $env:RELEASE_PLATFORM = "windows" }
if (-not $env:RELEASE_ARCH) { $env:RELEASE_ARCH = "x86_64" }

$ProfilePath = & bash ./scripts/ci/release-conan-profile.sh
$env:CONAN_PROFILE = $ProfilePath.Trim()

Write-Host "Using Conan profile at $env:CONAN_PROFILE"
Get-Content $env:CONAN_PROFILE

conan install . `
    --output-folder=$BuildDir `
    --build=missing `
    -s "build_type=$BuildType" `
    -pr $env:CONAN_PROFILE `
    -pr:b $env:CONAN_PROFILE

cmake --preset conan-release `
    -DBUILD_TESTING=$BuildTesting `
    -DBUILD_FUZZER=OFF `
    -DBUILD_COVERAGE=OFF `
    -DBUILD_CACHE=ON

cmake --build --preset conan-release --target beez --parallel

$BinaryPath = Join-Path $BuildDir "build/$BuildType/bin/beez.exe"
if (-not (Test-Path $BinaryPath)) {
    throw "release binary not found at $BinaryPath"
}

Write-Host "Built $BinaryPath"
