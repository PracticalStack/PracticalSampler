param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$CleanExisting
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildAppRoot = Join-Path $repoRoot "build\vs2022-$($Configuration.ToLowerInvariant())\app"
$targetRoot = Join-Path $env:CommonProgramFiles "VST3"
$targetBundle = Join-Path $targetRoot "Practical Sampler.vst3"
$legacyTargetBundle = Join-Path $targetRoot "Decent Rhapsody Studio.vst3"

$legacySourceBundles = @(Get-ChildItem -Path $buildAppRoot -Recurse -Directory -Filter "Decent Rhapsody Studio.vst3" |
    Where-Object { $_.FullName -like "*\\$Configuration\\VST3\\Decent Rhapsody Studio.vst3" })

if ($legacySourceBundles.Count -gt 0) {
    throw "A legacy same-CID VST3 bundle remains under '$buildAppRoot'. Run a clean $Configuration build before installing."
}

$sourceBundle = Join-Path $buildAppRoot "drs_plugin_bundle_artefacts\$Configuration\VST3\Practical Sampler.vst3"

Write-Host "== Practical Sampler VST3 install =="
Write-Host "Source bundle: $sourceBundle"
Write-Host "Target bundle: $targetBundle"

if (-not $sourceBundle -or -not (Test-Path $sourceBundle)) {
    throw "Built VST3 bundle was not found under '$buildAppRoot'. Build the plugin first with tools\\bootstrap-windows.ps1 or cmake --build --preset build-$($Configuration.ToLowerInvariant()) --target DecentRhapsodyStudioPlugin."
}

if (-not (Test-Path $targetRoot)) {
    throw "The system VST3 directory '$targetRoot' does not exist."
}

$resolvedTargetRoot = [IO.Path]::GetFullPath($targetRoot).TrimEnd('\')
foreach ($bundlePath in @($targetBundle, $legacyTargetBundle)) {
    $resolvedBundlePath = [IO.Path]::GetFullPath($bundlePath)
    if ([IO.Path]::GetDirectoryName($resolvedBundlePath).TrimEnd('\') -ne $resolvedTargetRoot) {
        throw "Refusing to modify VST3 bundle outside '$resolvedTargetRoot': $resolvedBundlePath"
    }
}

if (Test-Path $legacyTargetBundle) {
    Remove-Item -LiteralPath $legacyTargetBundle -Recurse -Force
    Write-Host "Removed legacy VST3 bundle: $legacyTargetBundle"
}

if ($CleanExisting -and (Test-Path $targetBundle)) {
    Remove-Item -LiteralPath $targetBundle -Recurse -Force
}

Copy-Item -LiteralPath $sourceBundle -Destination $targetRoot -Recurse -Force

Write-Host "Installed VST3 bundle to $targetBundle"
