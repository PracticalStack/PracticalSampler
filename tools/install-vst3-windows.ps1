param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$CleanExisting
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildAppRoot = Join-Path $repoRoot "build\vs2022-$($Configuration.ToLowerInvariant())\app"
$targetRoot = Join-Path $env:CommonProgramFiles "VST3"
$targetBundle = Join-Path $targetRoot "Decent Rhapsody Studio.vst3"

$sourceBundle = Get-ChildItem -Path $buildAppRoot -Recurse -Directory -Filter "Decent Rhapsody Studio.vst3" |
    Where-Object { $_.FullName -like "*\\$Configuration\\VST3\\Decent Rhapsody Studio.vst3" } |
    Select-Object -First 1 -ExpandProperty FullName

Write-Host "== Decent Rhapsody Studio VST3 install =="
Write-Host "Source bundle: $sourceBundle"
Write-Host "Target bundle: $targetBundle"

if (-not $sourceBundle -or -not (Test-Path $sourceBundle)) {
    throw "Built VST3 bundle was not found under '$buildAppRoot'. Build the plugin first with tools\\bootstrap-windows.ps1 or cmake --build --preset build-$($Configuration.ToLowerInvariant()) --target DecentRhapsodyStudioPlugin."
}

if (-not (Test-Path $targetRoot)) {
    throw "The system VST3 directory '$targetRoot' does not exist."
}

if ($CleanExisting -and (Test-Path $targetBundle)) {
    Remove-Item -LiteralPath $targetBundle -Recurse -Force
}

Copy-Item -LiteralPath $sourceBundle -Destination $targetRoot -Recurse -Force

Write-Host "Installed VST3 bundle to $targetBundle"
