param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$SkipBuild,

    [switch]$RunTests
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$configurePreset = if ($Configuration -eq "Debug") { "vs2022-debug" } else { "vs2022-release" }
$buildPreset = if ($Configuration -eq "Debug") { "build-debug" } else { "build-release" }
$testPreset = if ($Configuration -eq "Debug") { "test-debug" } else { "test-release" }
$vsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat was not found at '$vsDevCmd'. Verify that Visual Studio 2022 Community with Desktop development with C++ is installed."
}

Write-Host "== Practical Sampler Windows bootstrap =="
Write-Host "Repo root: $root"
Write-Host "Configuration preset: $configurePreset"
Write-Host "Run smoke tests: $RunTests"
Write-Host "Developer shell: $vsDevCmd"

Push-Location $root
try {
    $configureCommand = "cmake --preset $configurePreset"
    $bootstrapTargets = "drs_hise_frontend_plugin_probe DecentRhapsodyStudioApp DecentRhapsodyStudioPlugin"
    $smokeBuildCommand = "cmake --build --preset $buildPreset --target $bootstrapTargets drs_phase0_smoke_tests"
    $fullTestBuildCommand = "cmake --build --preset $buildPreset --target $bootstrapTargets drs_all_tests"
    $testCommand = "ctest --preset $testPreset"

    $fullCommand = if ($SkipBuild) {
        "call `"$vsDevCmd`" -arch=amd64 && $configureCommand"
    } elseif ($RunTests) {
        "call `"$vsDevCmd`" -arch=amd64 && $configureCommand && $fullTestBuildCommand && $testCommand"
    } else {
        "call `"$vsDevCmd`" -arch=amd64 && $configureCommand && $smokeBuildCommand"
    }

    cmd /c $fullCommand

    if ($LASTEXITCODE -ne 0) {
        throw "Bootstrap command failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
