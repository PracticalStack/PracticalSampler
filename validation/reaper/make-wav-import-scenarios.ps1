$validationRoot = $PSScriptRoot
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $validationRoot '..\..'))
$sourceStatePath = Join-Path $validationRoot 'active.hoststate.json'
$sourceProjectPath = Join-Path $repositoryRoot 'content\runtime\phase2\authoring-foundation\reference-project\phase2-authoring-foundation.drsproj'
$scenarioRoot = Join-Path $validationRoot 'wav-import-scenarios'
[IO.Directory]::CreateDirectory($scenarioRoot) | Out-Null

function Set-NativeProjectPaths {
    param(
        [object] $Project,
        [string] $ScenarioDirectory
    )

    $nativeSamplesRoot = Join-Path $repositoryRoot 'content\samples'
    $nativeInstrumentManifest = Join-Path $repositoryRoot 'content\runtime\phase1\reference-corpus\tiny-open-instrument\tiny-open-instrument.drinst'
    $Project.contentRoot = ([IO.Path]::GetRelativePath($ScenarioDirectory, $nativeSamplesRoot)).Replace('\', '/')
    $Project.defaultInstrumentManifest = ([IO.Path]::GetRelativePath($ScenarioDirectory, $nativeInstrumentManifest)).Replace('\', '/')
}

function Write-WavImportScenario {
    param(
        [string] $Name,
        [string[]] $SamplePaths
    )

    $scenarioDirectory = Join-Path $scenarioRoot $Name
    [IO.Directory]::CreateDirectory($scenarioDirectory) | Out-Null

    $project = Get-Content -LiteralPath $sourceProjectPath -Raw | ConvertFrom-Json
    if ($project.sampleSources.Count -ne $SamplePaths.Count) {
        throw "Scenario '$Name' expected $($project.sampleSources.Count) sample paths but received $($SamplePaths.Count)."
    }

    $project.displayName = "$($project.displayName) ($Name)"
    Set-NativeProjectPaths -Project $project -ScenarioDirectory $scenarioDirectory
    for ($index = 0; $index -lt $SamplePaths.Count; ++$index) {
        $project.sampleSources[$index].path = $SamplePaths[$index]
    }

    $projectPath = Join-Path $scenarioDirectory 'phase2-authoring-foundation.drsproj'
    [IO.File]::WriteAllText(
        $projectPath,
        ($project | ConvertTo-Json -Depth 100) + "`n",
        [Text.UTF8Encoding]::new($false))

    $state = Get-Content -LiteralPath $sourceStatePath -Raw | ConvertFrom-Json
    $state.projectBinding.manifestPath = [IO.Path]::GetFullPath($projectPath)
    $state.projectBinding.contentRootHint = [IO.Path]::GetFullPath($scenarioDirectory)
    $state.projectBinding.PSObject.Properties.Remove('portableRelativePath')

    $statePath = Join-Path $validationRoot "$Name.hoststate.json"
    [IO.File]::WriteAllText(
        $statePath,
        ($state | ConvertTo-Json -Depth 100) + "`n",
        [Text.UTF8Encoding]::new($false))

    & (Join-Path $validationRoot 'inject-host-state.ps1') `
        -BaselineProject (Join-Path $validationRoot 'baseline.rpp') `
        -HostState $statePath `
        -OutputProject (Join-Path $validationRoot "$Name.rpp")
}

Write-WavImportScenario -Name 'wav-import-missing-local' -SamplePaths @(
    'E:\Dev\Cpp\VST\DecentRhapsody\PracticalSampler\content\samples\missing-local-a3.wav',
    'E:\Dev\Cpp\VST\DecentRhapsody\PracticalSampler\content\samples\missing-local-a4.wav'
)

Write-WavImportScenario -Name 'wav-import-removable-media' -SamplePaths @(
    'R:\RemovedMedia\DRS_Sine_A3.wav',
    'R:\RemovedMedia\DRS_TriangleLead_A4.wav'
)

Write-WavImportScenario -Name 'wav-import-network-media' -SamplePaths @(
    '\\offline-host\drs\DRS_Sine_A3.wav',
    '\\offline-host\drs\DRS_TriangleLead_A4.wav'
)
