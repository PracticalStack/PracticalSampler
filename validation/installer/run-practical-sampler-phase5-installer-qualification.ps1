param(
    [string] $InstallerPath
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if ([string]::IsNullOrWhiteSpace($InstallerPath)) {
    $InstallerPath = Join-Path $repositoryRoot 'build\installer\PracticalSampler-Setup-0.1.0-phase5.exe'
}
$InstallerPath = [IO.Path]::GetFullPath($InstallerPath)

$appRoot = [IO.Path]::GetFullPath('C:\Program Files\Practical Sampler')
$vst3Root = [IO.Path]::GetFullPath('C:\Program Files\Common Files\VST3')
$currentVst3Path = [IO.Path]::GetFullPath((Join-Path $vst3Root 'Practical Sampler.vst3'))
$legacyVst3Path = [IO.Path]::GetFullPath((Join-Path $vst3Root 'Decent Rhapsody Studio.vst3'))
$currentExePath = [IO.Path]::GetFullPath((Join-Path $appRoot 'Practical Sampler.exe'))
$legacyExePath = [IO.Path]::GetFullPath((Join-Path $appRoot 'Decent Rhapsody Studio.exe'))
$startMenuRoot = [IO.Path]::GetFullPath('C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Practical Sampler')
$sourceExePath = Join-Path $repositoryRoot 'build\vs2022-release\app\DecentRhapsodyStudioApp_artefacts\Release\Practical Sampler.exe'
$sourceModulePath = Join-Path $repositoryRoot 'build\vs2022-release\app\drs_plugin_bundle_artefacts\Release\VST3\Practical Sampler.vst3\Contents\x86_64-win\Practical Sampler.vst3'
$sourceBundlePath = Join-Path $repositoryRoot 'build\vs2022-release\app\drs_plugin_bundle_artefacts\Release\VST3\Practical Sampler.vst3'
$installerOutputRoot = Join-Path $repositoryRoot 'build\installer'
$sentinelInstallerPath = Join-Path $installerOutputRoot 'PracticalSampler-Legacy-Sentinel.exe'
$evidencePath = Join-Path $PSScriptRoot 'phase5-installer-qualification.json'

if (-not (Test-Path -LiteralPath $InstallerPath -PathType Leaf)) {
    throw "Phase 5 installer not found: '$InstallerPath'."
}
foreach ($source in @($sourceExePath, $sourceModulePath)) {
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Release source artifact not found: '$source'."
    }
}
if ($appRoot -ne 'C:\Program Files\Practical Sampler' -or
    $vst3Root -ne 'C:\Program Files\Common Files\VST3' -or
    $startMenuRoot -ne 'C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Practical Sampler') {
    throw 'Resolved installer qualification paths differ from the explicit approved targets.'
}

function Invoke-SilentExecutable {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [string[]] $Arguments
    )
    $process = Start-Process -FilePath $Path -ArgumentList $Arguments -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "'$Path' exited with code $($process.ExitCode)."
    }
}

function Remove-QualificationInstall {
    $uninstaller = Join-Path $appRoot 'unins000.exe'
    if (Test-Path -LiteralPath $uninstaller -PathType Leaf) {
        Invoke-SilentExecutable $uninstaller @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART')
    }
    foreach ($target in @($appRoot, $currentVst3Path, $legacyVst3Path, $startMenuRoot)) {
        if (Test-Path -LiteralPath $target) {
            Remove-Item -LiteralPath $target -Recurse -Force
        }
    }
}

function Invoke-Installer {
    Invoke-SilentExecutable $InstallerPath @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-')
}

function New-LegacySentinelInstaller {
    $isccCandidates = @(
        'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
        'C:\Program Files\Inno Setup 6\ISCC.exe'
    )
    $iscc = $isccCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($iscc)) { throw 'Inno Setup 6 is required for the legacy upgrade simulation.' }
    $sentinelDefinition = Join-Path $PSScriptRoot 'phase5-legacy-sentinel.iss'
    & $iscc '/Qp' "/DDRSCurrentExe=$sourceExePath" "/DDRSCurrentVst3=$sourceBundlePath" "/DDRSOutputDir=$installerOutputRoot" $sentinelDefinition
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $sentinelInstallerPath -PathType Leaf)) {
        throw 'The legacy qualification sentinel installer could not be built.'
    }
}

function Get-InstalledRegistration {
    $registrations = @(@(
        Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*' -ErrorAction SilentlyContinue
        Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*' -ErrorAction SilentlyContinue
    ) | Where-Object DisplayName -eq 'Practical Sampler')
    if ($registrations.Count -ne 1) {
        throw "Expected one Practical Sampler uninstall registration; found $($registrations.Count)."
    }
    return $registrations[0]
}

function Assert-CurrentInstall {
    param([Parameter(Mandatory)] [string] $Stage)
    if (-not (Test-Path -LiteralPath $currentExePath -PathType Leaf)) { throw "${Stage}: current standalone is missing." }
    if (-not (Test-Path -LiteralPath $currentVst3Path -PathType Container)) { throw "${Stage}: current VST3 is missing." }
    if (Test-Path -LiteralPath $legacyExePath) { throw "${Stage}: legacy standalone remains installed." }
    if (Test-Path -LiteralPath $legacyVst3Path) { throw "${Stage}: legacy VST3 remains installed." }

    $modulePath = Join-Path $currentVst3Path 'Contents\x86_64-win\Practical Sampler.vst3'
    if ((Get-FileHash -LiteralPath $currentExePath -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $sourceExePath -Algorithm SHA256).Hash) {
        throw "${Stage}: installed standalone hash differs from the Release source."
    }
    if ((Get-FileHash -LiteralPath $modulePath -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $sourceModulePath -Algorithm SHA256).Hash) {
        throw "${Stage}: installed VST3 module hash differs from the Release source."
    }

    $registration = Get-InstalledRegistration
    if ($registration.Publisher -ne 'Practical Sampler Project') { throw "${Stage}: publisher metadata is incorrect." }
    if ($registration.DisplayVersion -ne '0.1.0-phase5') { throw "${Stage}: installer version is incorrect." }
    $shortcut = Join-Path $startMenuRoot 'Practical Sampler.lnk'
    if (-not (Test-Path -LiteralPath $shortcut -PathType Leaf)) { throw "${Stage}: current Start Menu shortcut is missing." }

    return [ordered]@{
        stage = $Stage
        product = $registration.DisplayName
        publisher = $registration.Publisher
        version = $registration.DisplayVersion
        currentStandaloneSha256 = (Get-FileHash -LiteralPath $currentExePath -Algorithm SHA256).Hash
        currentVst3ModuleSha256 = (Get-FileHash -LiteralPath $modulePath -Algorithm SHA256).Hash
        currentShortcut = $shortcut
        legacyStandaloneAbsent = -not (Test-Path -LiteralPath $legacyExePath)
        legacyVst3Absent = -not (Test-Path -LiteralPath $legacyVst3Path)
    }
}

Remove-QualificationInstall
Invoke-Installer
$clean = Assert-CurrentInstall 'clean-install'

New-LegacySentinelInstaller
Invoke-SilentExecutable $sentinelInstallerPath @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-')
if (-not (Test-Path -LiteralPath $legacyExePath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $legacyVst3Path -PathType Container)) {
    throw 'Could not establish the simulated legacy artifact state.'
}
Invoke-Installer
$upgrade = Assert-CurrentInstall 'legacy-upgrade'

$evidence = [ordered]@{
    schemaName = 'drs.practicalSampler.phase5InstallerQualification'
    schemaVersion = 1
    capturedAtUtc = [DateTime]::UtcNow.ToString('o')
    installerPath = $InstallerPath
    installerSha256 = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash
    legacySentinelInstallerSha256 = (Get-FileHash -LiteralPath $sentinelInstallerPath -Algorithm SHA256).Hash
    cleanInstall = $clean
    legacyUpgrade = $upgrade
}
[IO.File]::WriteAllText($evidencePath, ($evidence | ConvertTo-Json -Depth 10) + "`n", [Text.UTF8Encoding]::new($false))
Get-Content -LiteralPath $evidencePath
