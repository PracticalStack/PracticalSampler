param(
    [string] $ReaperPath = 'C:\Program Files\REAPER (x64)\reaper.exe',
    [int] $TimeoutSeconds = 30
)

$ErrorActionPreference = 'Stop'

$validationRoot = $PSScriptRoot
$configPath = Join-Path $validationRoot 'reaper.ini'
$scriptPath = Join-Path $validationRoot 'validate-wav-import-startup.lua'
$scenarios = @(
    'wav-import-missing-local',
    'wav-import-removable-media',
    'wav-import-network-media'
)

foreach ($name in $scenarios) {
    $projectPath = Join-Path $validationRoot ($name + '.rpp')
    $evidencePath = Join-Path $validationRoot ($name + '.wav-import-evidence.txt')
    $chunkPath = Join-Path $validationRoot ($name + '.wav-import-track-chunks.txt')

    Remove-Item -LiteralPath $evidencePath, $chunkPath -ErrorAction SilentlyContinue

    $process = Start-Process -FilePath $ReaperPath `
        -ArgumentList @('-cfgfile', $configPath, $projectPath, $scriptPath) `
        -WindowStyle Hidden `
        -PassThru

    try {
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        while ((Get-Date) -lt $deadline -and -not (Test-Path -LiteralPath $evidencePath)) {
            Start-Sleep -Milliseconds 250
            if ($process.HasExited) {
                break
            }
        }

        if (-not (Test-Path -LiteralPath $evidencePath)) {
            throw "Timed out waiting for $name evidence."
        }
    }
    finally {
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
        }
    }
}

Get-ChildItem -LiteralPath $validationRoot -Filter '*.wav-import-evidence.txt' |
    Sort-Object Name |
    Select-Object Name, Length, LastWriteTime
