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

function Resolve-ConanVsVersion {
    if ($env:VS_VERSION) {
        return $env:VS_VERSION
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "vswhere not found; set VS_VERSION explicitly for the Conan profile"
    }

    $productLine = (& $vswhere -latest -property catalog_productLineVersion).Trim()
    switch ($productLine) {
        "2022" { return "17" }
        "18" { return "18" }
        "2026" { return "18" }
        default {
            $installationVersion = (& $vswhere -latest -property installationVersion).Trim()
            $major = $installationVersion.Split(".")[0]
            if ($major -eq "17") { return "17" }
            if ([int]$major -ge 18) { return "18" }
            throw "unsupported Visual Studio product line '$productLine' (installationVersion=$installationVersion)"
        }
    }
}

New-Item -ItemType Directory -Force -Path $CiDir | Out-Null

if ($Platform -ne "windows") {
    throw "release-conan-profile.ps1 supports RELEASE_PLATFORM=windows only"
}

$VsVersion = Resolve-ConanVsVersion
$MsvcVersion = if ($env:MSVC_VERSION) {
    $env:MSVC_VERSION
} elseif ($VsVersion -eq "17") {
    "193"
} else {
    "194"
}

Write-Host "Using Conan VS version $VsVersion (MSVC $MsvcVersion, arch $ConanArch)"

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
tools.microsoft.msbuild:vs_version=$VsVersion
"@ | Set-Content -Path $ProfilePath -Encoding ascii -NoNewline
Add-Content -Path $ProfilePath -Value ""

Write-Output $ProfilePath
