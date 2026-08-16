param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$SkipBuild,

    [string]$AppVersion,

    [string]$OutputDir
)

$ErrorActionPreference = "Stop"

function Get-ProjectVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CMakeListsPath
    )

    $content = Get-Content -Path $CMakeListsPath -Raw
    $match = [regex]::Match($content, "project\s*\(\s*DecentRhapsodyStudio\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)")

    if ($match.Success) {
        return $match.Groups[1].Value
    }

    return "0.1.0"
}

function Find-InnoSetupCompiler {
    $candidates = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "C:\Program Files (x86)\Inno Setup 5\ISCC.exe",
        "C:\Program Files\Inno Setup 5\ISCC.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Find-BuiltVst3Bundle {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildAppRoot,

        [Parameter(Mandatory = $true)]
        [string]$Configuration
    )

    $legacyBundles = @(Get-ChildItem -Path $BuildAppRoot -Recurse -Directory -Filter "Decent Rhapsody Studio.vst3" |
        Where-Object { $_.FullName -like "*\$Configuration\VST3\Decent Rhapsody Studio.vst3" })

    if ($legacyBundles.Count -gt 0) {
        throw "A legacy same-CID VST3 bundle remains under '$BuildAppRoot'. Run a clean $Configuration build before packaging."
    }

    $bundle = Join-Path $BuildAppRoot "drs_plugin_bundle_artefacts\$Configuration\VST3\Practical Sampler.vst3"
    if (Test-Path -LiteralPath $bundle -PathType Container) {
        return $bundle
    }

    return $null
}

function Find-StandaloneDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildAppRoot,

        [Parameter(Mandatory = $true)]
        [string]$Configuration
    )

    $exe = Get-ChildItem -Path $BuildAppRoot -Recurse -File -Filter "Practical Sampler.exe" |
        Where-Object {
            $_.FullName -like "*\$Configuration\Standalone\Practical Sampler.exe" -or
            $_.FullName -like "*\$Configuration\Practical Sampler.exe"
        } |
        Select-Object -First 1

    if ($null -eq $exe) {
        return $null
    }

    return $exe.Directory.FullName
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildAppRoot = Join-Path $repoRoot "build\vs2022-$($Configuration.ToLowerInvariant())\app"
$projectVersion = Get-ProjectVersion -CMakeListsPath (Join-Path $repoRoot "CMakeLists.txt")
$resolvedAppVersion = if ([string]::IsNullOrWhiteSpace($AppVersion)) { "$projectVersion-tester" } else { $AppVersion }
$resolvedOutputDir = if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    Join-Path $repoRoot "build\installer"
} else {
    $OutputDir
}

Write-Host "== Practical Sampler tester installer =="
Write-Host "Repo root: $repoRoot"
Write-Host "Configuration: $Configuration"
Write-Host "Installer version: $resolvedAppVersion"

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "bootstrap-windows.ps1") -Configuration $Configuration

    if ($LASTEXITCODE -ne 0) {
        throw "Build step failed with exit code $LASTEXITCODE."
    }
}

$vst3Bundle = Find-BuiltVst3Bundle -BuildAppRoot $buildAppRoot -Configuration $Configuration

if (-not $vst3Bundle -or -not (Test-Path $vst3Bundle)) {
    throw "Built VST3 bundle was not found under '$buildAppRoot'. Build DecentRhapsodyStudioPlugin first."
}

$standaloneDir = Find-StandaloneDirectory -BuildAppRoot $buildAppRoot -Configuration $Configuration
$hasStandalone = if ($standaloneDir -and (Test-Path (Join-Path $standaloneDir "Practical Sampler.exe"))) { 1 } else { 0 }
$iscc = Find-InnoSetupCompiler

if (-not $iscc) {
    throw "Inno Setup compiler (ISCC.exe) was not found. Install Inno Setup 6, then rerun this script."
}

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$issPath = Join-Path $PSScriptRoot "drs-tester-installer.iss"
$arguments = @(
    "/Qp",
    "/DAppVersion=$resolvedAppVersion",
    "/DDRSVst3Source=$vst3Bundle",
    "/DDRSOutputDir=$resolvedOutputDir",
    "/DDRSHasStandalone=$hasStandalone"
)

if ($hasStandalone -eq 1) {
    $arguments += "/DDRSStandaloneDir=$standaloneDir"
}

$arguments += $issPath

Write-Host "VST3 source: $vst3Bundle"

if ($hasStandalone -eq 1) {
    Write-Host "Standalone source: $standaloneDir"
} else {
    Write-Host "Standalone source: not found, packaging VST3 only"
}

Write-Host "Output directory: $resolvedOutputDir"
Write-Host "Using Inno Setup: $iscc"

& $iscc @arguments

if ($LASTEXITCODE -ne 0) {
    throw "Installer build failed with exit code $LASTEXITCODE."
}

Write-Host "Tester installer created successfully."
