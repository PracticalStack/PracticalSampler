param(
    [string] $PianoLiteProjectPath = (Join-Path $env:USERPROFILE 'Documents\DecentRhapsodyStudio\PianoLite\PianoLite.drsproj'),
    [string] $ReaperPath = 'C:\Program Files\REAPER (x64)\reaper.exe',
    [int] $TimeoutSeconds = 90,
    [string[]] $SampleRates = @(44100, 48000),
    [int[]] $BlockSizes = @(128, 256, 512)
)

$ErrorActionPreference = 'Stop'

$validationRoot = $PSScriptRoot
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $validationRoot '..\..'))
$templateConfigPath = Join-Path $validationRoot 'reaper.ini'
$scenarioScriptPath = Join-Path $validationRoot 'validate-scenario.lua'
$baselineProjectPath = Join-Path $validationRoot 'baseline.rpp'
$captureToolPath = Join-Path $repositoryRoot 'build\vs2022-debug\tests\drs_host_project_recall_tests_artefacts\Debug\drs_host_project_recall_tests.exe'
$bundlePath = Join-Path $repositoryRoot 'build\vs2022-debug\app\drs_plugin_bundle_artefacts\Debug\VST3'
$evidenceRoot = Join-Path $validationRoot 'piano-lite-qualification-evidence'
$capturedStatePath = Join-Path $evidenceRoot 'piano-lite.hoststate.json'
$scenarioRoot = Join-Path $evidenceRoot 'scenarios'

foreach ($requiredFile in @($PianoLiteProjectPath, $ReaperPath, $templateConfigPath,
                             $scenarioScriptPath, $baselineProjectPath, $captureToolPath)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required Piano Lite qualification input was not found at '$requiredFile'."
    }
}
if (-not (Test-Path -LiteralPath $bundlePath -PathType Container)) {
    throw "The compiled Debug VST3 bundle directory was not found at '$bundlePath'."
}
if (Get-Process reaper -ErrorAction SilentlyContinue) {
    throw 'REAPER is already running. Close every REAPER session before the isolated Piano Lite qualification matrix.'
}

[IO.Directory]::CreateDirectory($evidenceRoot) | Out-Null
[IO.Directory]::CreateDirectory($scenarioRoot) | Out-Null

$previousCaptureProject = $env:DRS_HOST_STATE_CAPTURE_PROJECT_PATH
$previousCapturePath = $env:DRS_HOST_STATE_CAPTURE_PATH
try {
    $env:DRS_HOST_STATE_CAPTURE_PROJECT_PATH = [IO.Path]::GetFullPath($PianoLiteProjectPath)
    $env:DRS_HOST_STATE_CAPTURE_PATH = $capturedStatePath
    & $captureToolPath
    if ($LASTEXITCODE -ne 0) {
        throw "Piano Lite host-state capture failed with exit code $LASTEXITCODE."
    }
}
finally {
    $env:DRS_HOST_STATE_CAPTURE_PROJECT_PATH = $previousCaptureProject
    $env:DRS_HOST_STATE_CAPTURE_PATH = $previousCapturePath
}

$capturedState = Get-Content -LiteralPath $capturedStatePath -Raw | ConvertFrom-Json
if (($capturedState.schemaName -ne 'drs.hostState') -or
    ($capturedState.projectBinding.manifestPath -ne ([IO.Path]::GetFullPath($PianoLiteProjectPath))) -or
    ($capturedState.presetState.selectedArticulationId -ne 'default') -or
    ($null -eq $capturedState.publishedState)) {
    throw 'Captured Piano Lite state is not a published project-bound default-articulation host state.'
}

foreach ($scenarioName in @('piano-lite-editor-open', 'piano-lite-editor-closed')) {
    & (Join-Path $validationRoot 'inject-host-state.ps1') `
        -BaselineProject $baselineProjectPath `
        -HostState $capturedStatePath `
        -OutputProject (Join-Path $scenarioRoot "$scenarioName.rpp")
}

function Set-IniValue {
    param([string] $Text, [string] $Key, [string] $Value)
    return [regex]::Replace($Text, "(?m)^$([regex]::Escape($Key))=.*$", "$Key=$Value")
}

function New-QualificationProject {
    param([string] $ScenarioName, [int] $SampleRate, [int] $BlockSize)

    $sourcePath = Join-Path $scenarioRoot ($ScenarioName + '.rpp')
    $destinationPath = Join-Path $evidenceRoot ("{0}-{1}-{2}.rpp" -f $ScenarioName, $SampleRate, $BlockSize)
    $projectText = Get-Content -LiteralPath $sourcePath -Raw
    $projectText = [regex]::Replace($projectText, '(?m)^  SAMPLERATE .*$', "  SAMPLERATE $SampleRate 0 0")
    [IO.File]::WriteAllText($destinationPath, $projectText, [Text.UTF8Encoding]::new($false))
    return $destinationPath
}

function Assert-QualificationEvidence {
    param([string] $EvidencePath)

    $values = @{}
    Get-Content -LiteralPath $EvidencePath | ForEach-Object {
        $pair = $_ -split '=', 2
        if ($pair.Count -eq 2) { $values[$pair[0]] = $pair[1] }
    }
    foreach ($required in @('validation_midi_inserted', 'nonzero_peak_observations',
                              'nonfinite_peak_observations', 'track.0.fx.0.enabled',
                              'track.0.fx.0.offline')) {
        if (-not $values.ContainsKey($required)) {
            throw "Qualification evidence '$EvidencePath' omitted '$required'."
        }
    }
    $passed = $values['validation_midi_inserted'] -eq 'true'
    $passed = $passed -and ([int]$values['nonzero_peak_observations'] -gt 0)
    $passed = $passed -and ([int]$values['nonfinite_peak_observations'] -eq 0)
    $passed = $passed -and ($values['track.0.fx.0.enabled'] -eq 'true')
    $passed = $passed -and ($values['track.0.fx.0.offline'] -eq 'false')
    if (-not $passed) {
        throw "Piano Lite REAPER qualification failed for '$EvidencePath': $($values | Out-String)"
    }
}

function Stop-IsolatedReaperProcesses {
    param([string] $ConfigPath)

    Get-CimInstance Win32_Process -Filter "Name='reaper.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -like "*$ConfigPath*" } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

$templateConfig = Get-Content -LiteralPath $templateConfigPath -Raw
$previousEvidenceDirectory = $env:DRS_REAPER_EVIDENCE_DIR
try {
    $env:DRS_REAPER_EVIDENCE_DIR = $evidenceRoot
    foreach ($sampleRate in $SampleRates) {
        foreach ($blockSize in $BlockSizes) {
            $configText = Set-IniValue $templateConfig 'vstpath64' $bundlePath
            $configText = Set-IniValue $configText 'dummy_srate' $sampleRate
            $configText = Set-IniValue $configText 'dummy_blocksize' $blockSize
            $configDirectory = Join-Path $evidenceRoot ("reaper-{0}-{1}" -f $sampleRate, $blockSize)
            [IO.Directory]::CreateDirectory($configDirectory) | Out-Null
            $configPath = Join-Path $configDirectory 'reaper.ini'
            [IO.File]::WriteAllText(
                $configPath,
                $configText,
                [Text.UTF8Encoding]::new($false))
            Remove-Item -LiteralPath (Join-Path $configDirectory 'reaper-vstplugins64.ini') `
                -ErrorAction SilentlyContinue

            foreach ($scenarioName in @('piano-lite-editor-open', 'piano-lite-editor-closed')) {
                $projectPath = New-QualificationProject $scenarioName $sampleRate $blockSize
                $projectName = [IO.Path]::GetFileNameWithoutExtension($projectPath)
                $evidencePath = Join-Path $evidenceRoot ($projectName + '.reaper-evidence.txt')
                $chunkPath = Join-Path $evidenceRoot ($projectName + '.restored-track-chunks.txt')
                Remove-Item -LiteralPath $evidencePath, $chunkPath -ErrorAction SilentlyContinue

                $process = Start-Process -FilePath $ReaperPath `
                    -ArgumentList @('-newinst', '-cfgfile', $configPath, $projectPath, $scenarioScriptPath) `
                    -WindowStyle Hidden `
                    -PassThru
                try {
                    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
                    while ((Get-Date) -lt $deadline -and -not (Test-Path -LiteralPath $evidencePath)) {
                        Start-Sleep -Milliseconds 250
                        if ($process.HasExited) { break }
                    }
                    if (-not (Test-Path -LiteralPath $evidencePath)) {
                        throw "Timed out waiting for '$projectName' Piano Lite qualification evidence."
                    }
                    Assert-QualificationEvidence $evidencePath
                }
                finally {
                    Stop-IsolatedReaperProcesses $configPath
                }
            }
        }
    }
}
finally {
    $env:DRS_REAPER_EVIDENCE_DIR = $previousEvidenceDirectory
}

Get-ChildItem -LiteralPath $evidenceRoot -Filter '*.reaper-evidence.txt' |
    Sort-Object Name |
    Select-Object Name, Length, LastWriteTime
