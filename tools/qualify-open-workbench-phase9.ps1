param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$presetSuffix = $Configuration.ToLowerInvariant()
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) { throw "Visual Studio C++ tools were not found." }
$devCmd = Join-Path $vsRoot "Common7\Tools\VsDevCmd.bat"

$targets = @(
    "drs_open_workbench_qualification",
    "drs_phase0_smoke_tests",
    "drs_vst3_host_state_qualification_tests",
    "drs_phase2_authoring_playback_integration_tests",
    "drs_sprint6_publish_contract_tests",
    "drs_sprint6_publish_contract_seam_tests",
    "drs_sprint6_publish_controller_tests",
    "drs_sprint6_publish_controller_integration_tests",
    "drs_sprint6_publish_scheduling_tests",
    "drs_sprint6_published_macro_binding_tests",
    "drs_sprint6_publish_shell_parity_tests",
    "drs_performance_mixer_s0_red_tests",
    "drs_ui_responsiveness_baseline",
    "drs_performance_responsiveness_tests",
    "DecentRhapsodyStudioApp",
    "drs_plugin_bundle"
)

$targetText = $targets -join " "
$buildCommand = '"' + $devCmd + '" -arch=x64 -host_arch=x64 && cmake --build --preset build-' + $presetSuffix + ' --target ' + $targetText + ' -j 2'
$tests = @(
    "drs.phase0.smoke",
    "drs.host_state.vst3_qualification",
    "drs.sprint6.publish_contract",
    "drs.sprint6.publish_contract_seams",
    "drs.sprint6.publish_controller",
    "drs.sprint6.publish_controller_integration",
    "drs.sprint6.publish_scheduling",
    "drs.sprint6.published_macro_binding",
    "drs.performance_mixer.s3.published_presentation_model",
    "drs.performance_mixer.s3.published_presentation_rename",
    "drs.performance_mixer.s5.republish_churn_realtime",
    "drs.sprint6.publish_shell_parity",
    "drs.phase2.mapping_workspace",
    "drs.phase2.waveform_preview",
    "drs.phase2.macro_routing",
    "drs.phase2.performance_bank",
    "drs.phase2.performance_ui",
    "drs.phase2.authoring_playback_integration",
    "drs.phase2.authoring_ui",
    "drs.open_workbench.phase0",
    "drs.open_workbench.phase1",
    "drs.open_workbench.phase2",
    "drs.open_workbench.phase3",
    "drs.open_workbench.phase4",
    "drs.open_workbench.phase5",
    "drs.open_workbench.phase6",
    "drs.open_workbench.phase7",
    "drs.open_workbench.phase8",
    "drs.open_workbench.phase9",
    "drs.performance_engine.s10.articulation_ui",
    "drs.phase2.repeated_structure_density",
    "drs.ui.performance_responsiveness"
)
if ($Configuration -eq "Release")
{
    # The optimized build is the authority for the 1,700-zone/180-second
    # responsiveness workload. Debug retains the focused Perform latency gate.
    $tests += "drs.ui.responsiveness_baseline"
}

Push-Location $repoRoot
try
{
    & cmd.exe /d /s /c $buildCommand
    if ($LASTEXITCODE -ne 0) { throw "Pass 03 convergence build matrix failed." }

    foreach ($test in $tests)
    {
        $escapedTest = [Regex]::Escape($test)
        $passed = $false
        for ($attempt = 1; $attempt -le 2 -and -not $passed; ++$attempt)
        {
            Start-Sleep -Milliseconds (250 * $attempt)
            ctest --preset "test-$presetSuffix" -R "^$escapedTest$" --output-on-failure --timeout 180
            $passed = $LASTEXITCODE -eq 0
        }
        if (-not $passed)
        {
            throw "Pass 03 convergence test failed twice: $test"
        }
    }
}
finally
{
    Pop-Location
}
