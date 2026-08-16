param(
    [string] $ReaperPath = 'C:\Program Files\REAPER (x64)\reaper.exe',
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',
    [int] $TimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'
$validationRoot = (Resolve-Path $PSScriptRoot).Path
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $validationRoot '..\..'))
$evidenceRoot = Join-Path $validationRoot 'phase5-identity-evidence'
$runtimeRoot = Join-Path ([IO.Path]::GetTempPath()) 'PracticalSamplerPhase5Reaper'
$scanRoot = Join-Path $runtimeRoot 'VST3'
$configPath = Join-Path $runtimeRoot 'reaper.ini'
$projectPath = Join-Path $evidenceRoot 'phase5-current-session.rpp'
$duplicatePath = Join-Path $evidenceRoot 'phase5-duplicate-session.rpp'
$createEvidencePath = Join-Path $evidenceRoot 'phase5-create-session.txt'
$restoreEvidencePath = Join-Path $evidenceRoot 'phase5-restore-session.txt'
$summaryPath = Join-Path $evidenceRoot 'phase5-identity-summary.json'
$bundlePath = Join-Path $repositoryRoot ("build\vs2022-{0}\app\drs_plugin_bundle_artefacts\{1}\VST3\Practical Sampler.vst3" -f $Configuration.ToLowerInvariant(), $Configuration)
$legacyBundleName = 'Decent Rhapsody Studio.vst3'
$legacyDisplayName = 'Decent Rhapsody Studio'

if (-not (Test-Path -LiteralPath $ReaperPath -PathType Leaf)) {
    throw "REAPER was not found at '$ReaperPath'."
}
if (Get-Process reaper -ErrorAction SilentlyContinue) {
    throw 'REAPER is already running. Close it before the isolated Phase 5 identity qualification.'
}
if (-not (Test-Path -LiteralPath $bundlePath -PathType Container)) {
    throw "The $Configuration Practical Sampler bundle was not found at '$bundlePath'."
}

$resolvedEvidenceRoot = [IO.Path]::GetFullPath($evidenceRoot)
$validationPrefix = $validationRoot.TrimEnd('\') + '\'
if (-not $resolvedEvidenceRoot.StartsWith($validationPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to recreate evidence outside '$validationRoot'."
}
if (Test-Path -LiteralPath $resolvedEvidenceRoot) {
    Remove-Item -LiteralPath $resolvedEvidenceRoot -Recurse -Force
}
[IO.Directory]::CreateDirectory($evidenceRoot) | Out-Null
$resolvedRuntimeRoot = [IO.Path]::GetFullPath($runtimeRoot)
$tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
if (-not $resolvedRuntimeRoot.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing to recreate the isolated REAPER runtime outside the system temporary directory.'
}
if (Test-Path -LiteralPath $resolvedRuntimeRoot) {
    Remove-Item -LiteralPath $resolvedRuntimeRoot -Recurse -Force
}
[IO.Directory]::CreateDirectory($scanRoot) | Out-Null
Copy-Item -LiteralPath $bundlePath -Destination (Join-Path $scanRoot 'Practical Sampler.vst3') -Recurse -Force

$legacyBundles = @(Get-ChildItem -LiteralPath $scanRoot -Recurse -Directory -Filter $legacyBundleName)
if ($legacyBundles.Count -ne 0) {
    throw "The isolated scan root contains the legacy same-CID bundle: $($legacyBundles.FullName -join ', ')"
}
$installedBundles = @(Get-ChildItem -LiteralPath $scanRoot -Directory -Filter '*.vst3')
if ($installedBundles.Count -ne 1 -or $installedBundles[0].Name -ne 'Practical Sampler.vst3') {
    throw "The isolated scan root must contain exactly one Practical Sampler bundle."
}

function Set-IniValue {
    param([string] $Text, [string] $Key, [string] $Value)
    if ($Text -match "(?m)^$([regex]::Escape($Key))=") {
        return [regex]::Replace($Text, "(?m)^$([regex]::Escape($Key))=.*$", "$Key=$Value")
    }
    return $Text + "`n$Key=$Value`n"
}

$configText = Get-Content -LiteralPath (Join-Path $validationRoot 'reaper.ini') -Raw
$configText = Set-IniValue $configText 'vstpath64' $scanRoot
$configText = Set-IniValue $configText 'dummy_srate' '48000'
$configText = Set-IniValue $configText 'dummy_blocksize' '256'
[IO.File]::WriteAllText($configPath, $configText, [Text.UTF8Encoding]::new($false))

function Stop-IsolatedReaperProcesses {
    Get-CimInstance Win32_Process -Filter "Name='reaper.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -like "*$configPath*" } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

function Invoke-IsolatedReaper {
    param(
        [Parameter(Mandatory)] [string[]] $Arguments,
        [Parameter(Mandatory)] [string] $ExpectedEvidence
    )

    Remove-Item -LiteralPath $ExpectedEvidence -Force -ErrorAction SilentlyContinue
    $process = Start-Process -FilePath $ReaperPath -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    try {
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        while ((Get-Date) -lt $deadline -and -not (Test-Path -LiteralPath $ExpectedEvidence)) {
            Start-Sleep -Milliseconds 250
            if ($process.HasExited) { break }
        }
        if (-not (Test-Path -LiteralPath $ExpectedEvidence)) {
            throw "Timed out waiting for '$ExpectedEvidence'."
        }
        $firstLine = Get-Content -LiteralPath $ExpectedEvidence -First 1
        if ($firstLine -ne 'status=pass') {
            throw "REAPER qualification failed: $((Get-Content -LiteralPath $ExpectedEvidence -Raw).Trim())"
        }
    }
    finally {
        Stop-IsolatedReaperProcesses
    }
}

function Read-KeyValues {
    param([Parameter(Mandatory)] [string] $Path)
    $values = @{}
    Get-Content -LiteralPath $Path | ForEach-Object {
        $pair = $_ -split '=', 2
        if ($pair.Count -eq 2) { $values[$pair[0]] = $pair[1] }
    }
    return $values
}

$previousEvidence = $env:DRS_PHASE5_EVIDENCE_DIR
$previousProject = $env:DRS_PHASE5_PROJECT_PATH
$previousDuplicate = $env:DRS_PHASE5_DUPLICATE_PATH
try {
    $env:DRS_PHASE5_EVIDENCE_DIR = $evidenceRoot
    $env:DRS_PHASE5_PROJECT_PATH = $projectPath
    $env:DRS_PHASE5_DUPLICATE_PATH = $duplicatePath

    Invoke-IsolatedReaper -Arguments @(
        '-newinst', '-cfgfile', $configPath, '-new',
        (Join-Path $validationRoot 'phase5-create-session.lua')) -ExpectedEvidence $createEvidencePath
    Invoke-IsolatedReaper -Arguments @(
        '-newinst', '-cfgfile', $configPath, $projectPath,
        (Join-Path $validationRoot 'phase5-restore-session.lua')) -ExpectedEvidence $restoreEvidencePath
}
finally {
    $env:DRS_PHASE5_EVIDENCE_DIR = $previousEvidence
    $env:DRS_PHASE5_PROJECT_PATH = $previousProject
    $env:DRS_PHASE5_DUPLICATE_PATH = $previousDuplicate
}

$create = Read-KeyValues $createEvidencePath
$restore = Read-KeyValues $restoreEvidencePath
$expectedFxName = 'VST3i: Practical Sampler (Practical Sampler Project)'
$requiredEqualKeys = @(
    'fx_name', 'parameter_count', 'tone_name', 'motion_name', 'motion_value',
    'automation_point_count', 'automation_point_0_time', 'automation_point_0_value',
    'automation_point_1_time', 'automation_point_1_value'
)
foreach ($key in $requiredEqualKeys) {
    if ($create[$key] -ne $restore[$key]) {
        throw "Saved/reopened value mismatch for '$key': create='$($create[$key])', restore='$($restore[$key])'."
    }
}
if ($create.fx_name -ne $expectedFxName) { throw "Unexpected hosted identity: '$($create.fx_name)'." }
if ([int]$create.parameter_count -lt 16) { throw 'The hosted plug-in exposed fewer than the 16 stable parameters.' }
if ($create.editor_open -ne 'true' -or $create.saved_editor_closed -ne 'true') { throw 'The create run did not prove both editor states.' }
if ($restore.editor_initially_open -ne 'false' -or $restore.editor_open_check -ne 'true') { throw 'The reopen run did not restore closed and then open its editor.' }
if ($restore.duplicate_track_count -ne '2' -or $restore.duplicate_independent -ne 'true') { throw 'Duplicate-instance independence failed.' }

$cachePath = Join-Path $runtimeRoot 'reaper-vstplugins64.ini'
if (-not (Test-Path -LiteralPath $cachePath)) { throw 'REAPER did not write an isolated VST3 scan cache.' }
$cacheText = Get-Content -LiteralPath $cachePath -Raw
$cacheEntries = @([regex]::Matches($cacheText, '(?m)^Practical_Sampler\.vst3=.*$'))
if ($cacheEntries.Count -ne 1) { throw "Expected exactly one Practical Sampler scan entry; found $($cacheEntries.Count)." }
if ($cacheText.Contains($legacyDisplayName)) { throw 'The isolated host cache still contains the legacy product display name.' }
if (-not $cacheText.Contains('ABCDEF019182FAEB4463726844727330')) { throw 'The isolated cache omitted the stable component CID.' }
[IO.File]::WriteAllText((Join-Path $evidenceRoot 'phase5-reaper-vstplugins64.ini'), $cacheText, [Text.UTF8Encoding]::new($false))

$createdProjectText = Get-Content -LiteralPath $projectPath -Raw
if (-not $createdProjectText.Contains('Practical Sampler (Practical Sampler Project)')) {
    throw 'The saved current-session project omitted the approved product/company identity.'
}
if ($createdProjectText.Contains($legacyDisplayName)) {
    throw 'The saved current-session project contains the legacy product display name.'
}

$summary = [ordered]@{
    schemaName = 'drs.practicalSampler.phase5HostIdentityQualification'
    schemaVersion = 1
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    reaperPath = $ReaperPath
    configuration = $Configuration
    isolatedScanRoot = $scanRoot
    installedBundles = @($installedBundles.Name)
    scanEntryCount = $cacheEntries.Count
    scannedFxName = $create.fx_name
    stableComponentCid = 'ABCDEF019182FAEB4463726844727330'
    parameterCount = [int]$create.parameter_count
    savedReopenedValuesMatch = $true
    editorOpenAndClosed = $true
    duplicateInstancesIndependent = $true
    currentProject = $projectPath
    duplicateProject = $duplicatePath
}
[IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 10) + "`n", [Text.UTF8Encoding]::new($false))
Remove-Item -LiteralPath $resolvedRuntimeRoot -Recurse -Force
Get-Content -LiteralPath $summaryPath
