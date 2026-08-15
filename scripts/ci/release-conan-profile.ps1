$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$CiDir = Join-Path $RootDir ".ci"
$ProfilePath = Join-Path $CiDir "release-conan-profile"

$Platform = if ($env:RELEASE_PLATFORM) { $env:RELEASE_PLATFORM } else { "windows" }
$Arch = if ($env:RELEASE_ARCH) { $env:RELEASE_ARCH } else { "x86_64" }
$BuildType = if ($env:RELEASE_BUILD_TYPE) { $env:RELEASE_BUILD_TYPE } else { "Release" }

$ConanArch = switch ($Arch) {
    "x86_64" { "x86_64" }
    "aarch64" { "armv8" }
    default { throw "unsupported RELEASE_ARCH=$Arch" }
}

New-Item -ItemType Directory -Force -Path $CiDir | Out-Null

if ($Platform -ne "windows") {
    throw "release-conan-profile.ps1 supports RELEASE_PLATFORM=windows only"
}

$MsvcVersion = if ($env:MSVC_VERSION) { $env:MSVC_VERSION } else { "194" }

@"
[settings]
arch=$ConanArch
build_type=$BuildType
compiler=msvc
compiler.version=$MsvcVersion
compiler.cppstd=20
compiler.runtime=dynamic
os=Windows

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
"@ | Set-Content -Path $ProfilePath -Encoding ascii -NoNewline
Add-Content -Path $ProfilePath -Value ""

Write-Output $ProfilePath
