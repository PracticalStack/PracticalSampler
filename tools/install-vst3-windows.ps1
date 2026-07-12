param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$CleanExisting
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$artefactRoot = Join-Path $repoRoot "build\vs2022-$($Configuration.ToLowerInvariant())\app\DecentRhapsodyStudioPlugin_artefacts\$Configuration\VST3"
$sourceBundle = Join-Path $artefactRoot "Decent Rhapsody Studio.vst3"
$targetRoot = Join-Path $env:CommonProgramFiles "VST3"
$targetBundle = Join-Path $targetRoot "Decent Rhapsody Studio.vst3"

Write-Host "== Decent Rhapsody Studio VST3 install =="
Write-Host "Source bundle: $sourceBundle"
Write-Host "Target bundle: $targetBundle"

if (-not (Test-Path $sourceBundle)) {
    throw "Built VST3 bundle was not found at '$sourceBundle'. Build the plugin first with tools\\bootstrap-windows.ps1."
}

if (-not (Test-Path $targetRoot)) {
    throw "The system VST3 directory '$targetRoot' does not exist."
}

if ($CleanExisting -and (Test-Path $targetBundle)) {
    Remove-Item -LiteralPath $targetBundle -Recurse -Force
}

Copy-Item -LiteralPath $sourceBundle -Destination $targetRoot -Recurse -Force

Write-Host "Installed VST3 bundle to $targetBundle"
