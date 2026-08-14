param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$IncludeVst3,
    [switch]$IncludeResponsiveness,
    [switch]$IncludeLargeInstrument
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$presetSuffix = $Configuration.ToLowerInvariant()
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) { throw "Visual Studio C++ tools were not found." }
$devCmd = Join-Path $vsRoot "Common7\Tools\VsDevCmd.bat"

$targets = @(
    "drs_open_workbench_phase0_tests",
    "drs_open_workbench_phase1_tests",
    "drs_open_workbench_phase2_tests",
    "drs_open_workbench_phase3_tests",
    "drs_open_workbench_phase4_tests",
    "drs_open_workbench_phase5_tests",
    "drs_phase2_authoring_ui_tests",
    "drs_phase2_mapping_workspace_tests",
    "drs_phase2_repeated_structure_density_tests",
    "drs_phase2_waveform_preview_tests",
    "drs_phase2_macro_routing_tests",
    "drs_phase2_performance_ui_tests",
    "drs_phase2_performance_bank_tests",
    "drs_performance_engine_s10_articulation_ui_tests"
)
if ($IncludeVst3)
{
    $targets += "drs_phase0_smoke_tests"
    $targets += "drs_vst3_host_state_qualification_tests"
    $targets += "drs_plugin_bundle"
}
if ($IncludeResponsiveness) { $targets += "drs_ui_responsiveness_baseline" }
if ($IncludeLargeInstrument) { $targets += "drs_large_instrument_qualification" }

$targetText = $targets -join " "
$buildCommand = '"' + $devCmd + '" -arch=x64 -host_arch=x64 && cmake --build --preset build-' + $presetSuffix + ' --target ' + $targetText + ' -j 2'
Push-Location $repoRoot
try
{
    & cmd.exe /d /s /c $buildCommand
    if ($LASTEXITCODE -ne 0) { throw "Open Workbench build matrix failed." }

    $testRegex = "drs\.(open_workbench\.phase[0-5]|phase2\.(authoring_ui|mapping_workspace|repeated_structure_density|waveform_preview|macro_routing|performance_ui|performance_bank)|performance_engine\.s10\.articulation_ui)"
    if ($IncludeVst3) { $testRegex = "($testRegex|drs\.(phase0\.smoke|host_state\.vst3_qualification))" }
    if ($IncludeResponsiveness) { $testRegex = "($testRegex|drs\.ui\.responsiveness_baseline)" }
    ctest --preset "test-$presetSuffix" -R $testRegex --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Open Workbench automated qualification failed." }

    if ($IncludeLargeInstrument)
    {
        $sfz = Join-Path $repoRoot "..\DemoSFVInstruments\AccurateSalamanderGrandPianoV6.2beta2_48khz24bit\sfz_daw\Accurate-SalamanderGrandPiano_flat.Recommended.sfz"
        $package = Join-Path $repoRoot "build\vs2022-$presetSuffix\tests\open-workbench-phase5-large-instrument.drpkg"
        $report = Join-Path $repoRoot "artifacts\open-workbench-phase5-large-instrument-$presetSuffix.md"
        $exe = Join-Path $repoRoot "build\vs2022-$presetSuffix\tests\drs_large_instrument_qualification.exe"
        New-Item -ItemType Directory -Force (Split-Path $report) | Out-Null
        try
        {
            & $exe $sfz $report $package
            if ($LASTEXITCODE -ne 0) { throw "Large-instrument qualification failed." }
        }
        finally
        {
            if (Test-Path -LiteralPath $package)
            {
                $resolvedPackage = (Resolve-Path -LiteralPath $package).Path
                $expectedPackage = [System.IO.Path]::GetFullPath($package)
                $expectedDirectory = [System.IO.Path]::GetFullPath(
                    (Join-Path $repoRoot "build\vs2022-$presetSuffix\tests"))
                $insideExpectedDirectory = $resolvedPackage.StartsWith(
                    $expectedDirectory, [System.StringComparison]::OrdinalIgnoreCase)
                if (($resolvedPackage -ne $expectedPackage) -or (-not $insideExpectedDirectory))
                {
                    throw "Refusing to remove an unexpected large-instrument package path."
                }
                Remove-Item -LiteralPath $resolvedPackage -Force
            }
        }
    }
}
finally
{
    Pop-Location
}
