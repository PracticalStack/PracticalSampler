param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("Verify", "Refresh")]
    [string]$Mode = "Verify"
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$configurePreset = if ($Configuration -eq "Debug") { "vs2022-debug" } else { "vs2022-release" }
$buildPreset = if ($Configuration -eq "Debug") { "build-debug" } else { "build-release" }
$testPreset = if ($Configuration -eq "Debug") { "test-debug" } else { "test-release" }
$vsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat was not found at '$vsDevCmd'. Verify that Visual Studio 2022 Community with Desktop development with C++ is installed."
}

Write-Host "== Phase 1 reference instrument package =="
Write-Host "Repo root: $repoRoot"
Write-Host "Configuration preset: $configurePreset"
Write-Host "Mode: $Mode"

Push-Location $repoRoot
try {
    $configureCommand = "cmake --preset $configurePreset"
    $buildCommand = "cmake --build --preset $buildPreset --target drs_phase1_runtime_fixture_tool"

    if ($Mode -eq "Verify") {
        $testCommand = "ctest --preset $testPreset -R drs.phase1.fixture_tool_verify --output-on-failure"
        $fullCommand = "call `"$vsDevCmd`" -arch=amd64 && $configureCommand && $buildCommand && $testCommand"
        cmd /c $fullCommand

        if ($LASTEXITCODE -ne 0) {
            throw "Reference package verification failed with exit code $LASTEXITCODE."
        }

        return
    }

    $fullCommand = "call `"$vsDevCmd`" -arch=amd64 && $configureCommand && $buildCommand"
    cmd /c $fullCommand

    if ($LASTEXITCODE -ne 0) {
        throw "Reference package refresh failed with exit code $LASTEXITCODE."
    }

    $tool = Get-ChildItem -Path ".\build" -Recurse -Filter "drs_phase1_runtime_fixture_tool.exe" | Select-Object -First 1 -ExpandProperty FullName
    if (-not $tool) {
        throw "Could not locate the built Phase 1 runtime fixture tool executable."
    }

    & $tool --write-reference-package
    if ($LASTEXITCODE -ne 0) {
        throw "Phase 1 runtime fixture tool refresh failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
