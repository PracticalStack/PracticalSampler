param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$configurePreset = if ($Configuration -eq "Debug") { "vs2022-debug" } else { "vs2022-release" }
$buildPreset = if ($Configuration -eq "Debug") { "build-debug" } else { "build-release" }
$vsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
$scenePath = Join-Path $repoRoot "content\runtime\phase1\benchmark-scenes\reference-playback-scene.json"

if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat was not found at '$vsDevCmd'. Verify that Visual Studio 2022 Community with Desktop development with C++ is installed."
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $repoRoot "build\vs2022-$($Configuration.ToLowerInvariant())\tests\phase1-benchmark-scene.json"
}

Write-Host "== Phase 1 benchmark scene =="
Write-Host "Repo root: $repoRoot"
Write-Host "Configuration preset: $configurePreset"
Write-Host "Scene: $scenePath"
Write-Host "Report: $OutputPath"

Push-Location $repoRoot
try {
    $configureCommand = "cmake --preset $configurePreset"
    $buildCommand = "cmake --build --preset $buildPreset --target drs_phase1_benchmark_scene"
    $fullCommand = "call `"$vsDevCmd`" -arch=amd64 && $configureCommand && $buildCommand"
    cmd /c $fullCommand

    if ($LASTEXITCODE -ne 0) {
        throw "Benchmark scene build failed with exit code $LASTEXITCODE."
    }

    $tool = Get-ChildItem -Path ".\build" -Recurse -Filter "drs_phase1_benchmark_scene.exe" | Select-Object -First 1 -ExpandProperty FullName
    if (-not $tool) {
        throw "Could not locate the built Phase 1 benchmark scene executable."
    }

    & $tool $scenePath $OutputPath
    if ($LASTEXITCODE -ne 0) {
        throw "Benchmark scene execution failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
