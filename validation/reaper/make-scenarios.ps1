$validationRoot = $PSScriptRoot
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $validationRoot '..\..'))
$sourceStatePath = Join-Path $validationRoot 'active.hoststate.json'
$sourceProjectPath = Join-Path $repositoryRoot 'content\runtime\phase2\authoring-foundation\reference-project\phase2-authoring-foundation.drsproj'
$scenarioRoot = Join-Path $validationRoot 'scenarios'
[IO.Directory]::CreateDirectory($scenarioRoot) | Out-Null

function Write-StateScenario {
    param(
        [string] $Name,
        [string] $ManifestPath,
        [string] $ContentRootHint
    )

    $state = Get-Content -LiteralPath $sourceStatePath -Raw | ConvertFrom-Json
    $state.projectBinding.manifestPath = [IO.Path]::GetFullPath($ManifestPath)
    $state.projectBinding.contentRootHint = [IO.Path]::GetFullPath($ContentRootHint)
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

foreach ($name in @('active-editor-open', 'active-editor-closed', 'duplicate-instances')) {
    Copy-Item -LiteralPath $sourceStatePath -Destination (Join-Path $validationRoot "$name.hoststate.json") -Force
    & (Join-Path $validationRoot 'inject-host-state.ps1') `
        -BaselineProject (Join-Path $validationRoot 'baseline.rpp') `
        -HostState (Join-Path $validationRoot "$name.hoststate.json") `
        -OutputProject (Join-Path $validationRoot "$name.rpp")
}

$movedPath = Join-Path $scenarioRoot 'moved-away\phase2-authoring-foundation.drsproj'
Write-StateScenario -Name 'moved-project' -ManifestPath $movedPath -ContentRootHint (Split-Path $movedPath)

$changedDirectory = Join-Path $scenarioRoot 'changed'
[IO.Directory]::CreateDirectory($changedDirectory) | Out-Null
$changedPath = Join-Path $changedDirectory 'phase2-authoring-foundation.drsproj'
$changed = Get-Content -LiteralPath $sourceProjectPath -Raw | ConvertFrom-Json
$changed.displayName = "$($changed.displayName) changed after DAW save"
[IO.File]::WriteAllText(
    $changedPath,
    ($changed | ConvertTo-Json -Depth 100) + "`n",
    [Text.UTF8Encoding]::new($false))
Write-StateScenario -Name 'changed-manifest' -ManifestPath $changedPath -ContentRootHint $changedDirectory

$missingDirectory = Join-Path $scenarioRoot 'missing-samples'
[IO.Directory]::CreateDirectory($missingDirectory) | Out-Null
$missingPath = Join-Path $missingDirectory 'phase2-authoring-foundation.drsproj'
Copy-Item -LiteralPath $sourceProjectPath -Destination $missingPath -Force
Write-StateScenario -Name 'missing-sample' -ManifestPath $missingPath -ContentRootHint $missingDirectory
