[CmdletBinding()]
param(
    [string]$RulesPath,
    [string]$OutputPath,
    [switch]$RepositoryOnly,
    [switch]$NoWrite
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($RulesPath)) {
    $RulesPath = Join-Path $PSScriptRoot 'practical-sampler-identity-rules.json'
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'docs\practical-sampler-phase0-identity-ledger.json'
}

function Get-NormalizedRelativePath {
    param(
        [Parameter(Mandatory)] [string]$Root,
        [Parameter(Mandatory)] [string]$Path
    )

    $absolutePath = if ([IO.Path]::IsPathRooted($Path)) {
        [IO.Path]::GetFullPath($Path)
    } else {
        [IO.Path]::GetFullPath((Join-Path $Root $Path))
    }
    $rootWithSeparator = $Root.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $rootUri = [Uri]$rootWithSeparator
    $pathUri = [Uri]$absolutePath
    return [Uri]::UnescapeDataString($rootUri.MakeRelativeUri($pathUri).ToString()).Replace('\', '/')
}

function Test-RuleMatch {
    param(
        [Parameter(Mandatory)] $Rule,
        [Parameter(Mandatory)] [string]$Identity,
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$Line
    )

    if ($Rule.PSObject.Properties.Name -contains 'identity' -and $Rule.identity -ne $Identity) {
        return $false
    }
    if ($Rule.PSObject.Properties.Name -contains 'pathRegex' -and $Path -notmatch $Rule.pathRegex) {
        return $false
    }
    if ($Rule.PSObject.Properties.Name -contains 'lineRegex' -and $Line -notmatch $Rule.lineRegex) {
        return $false
    }
    return $true
}

$rulesFile = (Resolve-Path -LiteralPath $RulesPath).Path
$rules = Get-Content -LiteralPath $rulesFile -Raw | ConvertFrom-Json
$repositoryRoot = (Resolve-Path (Split-Path $PSScriptRoot -Parent)).Path
$workspaceRoot = if ($RepositoryOnly) {
    $repositoryRoot
} else {
    (Resolve-Path (Join-Path $repositoryRoot $rules.workspaceRootRelativeToRepository)).Path
}

$legacyProductName = [string]$rules.legacyIdentity.productName
$legacyCompanyName = [string]$rules.legacyIdentity.companyName
$searchPattern = [regex]::Escape($legacyProductName) + '|' + [regex]::Escape($legacyCompanyName)

$rgArguments = @(
    '--json',
    '--hidden',
    '--glob', '!.git/**',
    '--glob', '!**/.git/**',
    '--glob', '!**/.vs/**',
    '--glob', '!**/build/**',
    '--glob', '!**/artifacts/**',
    '--glob', '!**/tmp/**',
    '--glob', '!**/practical-sampler-phase0-identity-ledger.json',
    '--regexp', $searchPattern,
    $workspaceRoot
)

$rawEvents = & rg @rgArguments
$rgExitCode = $LASTEXITCODE
if ($rgExitCode -gt 1) {
    throw "rg identity scan failed with exit code $rgExitCode."
}

$occurrences = [System.Collections.Generic.List[object]]::new()
foreach ($rawEvent in $rawEvents) {
    $event = $rawEvent | ConvertFrom-Json
    if ($event.type -ne 'match') {
        continue
    }

    $lineText = ([string]$event.data.lines.text).TrimEnd("`r", "`n")
    $relativePath = Get-NormalizedRelativePath -Root $workspaceRoot -Path ([string]$event.data.path.text)
    $excerpt = $lineText.Trim()
    if ($excerpt.Length -gt 220) {
        $excerpt = $excerpt.Substring(0, 217) + '...'
    }

    foreach ($submatch in @($event.data.submatches)) {
        $matchedText = [string]$submatch.match.text
        $identity = if ($matchedText -eq $legacyProductName) { 'productName' } else { 'companyName' }
        $matchedRule = $null
        foreach ($rule in $rules.rules) {
            if (Test-RuleMatch -Rule $rule -Identity $identity -Path $relativePath -Line $lineText) {
                $matchedRule = $rule
                break
            }
        }

        $classification = if ($null -eq $matchedRule) { 'UNCLASSIFIED' } else { [string]$matchedRule.classification }
        $ruleId = if ($null -eq $matchedRule) { '' } else { [string]$matchedRule.id }
        $owner = if ($null -eq $matchedRule) { '' } else { [string]$matchedRule.owner }
        $reason = if ($null -eq $matchedRule) { 'No classification rule matched.' } else { [string]$matchedRule.reason }

        $occurrences.Add([pscustomobject][ordered]@{
            path = $relativePath
            line = [int]$event.data.line_number
            column = [int]$submatch.start + 1
            identity = $identity
            matchedText = $matchedText
            classification = $classification
            rule = $ruleId
            owner = $owner
            reason = $reason
            excerpt = $excerpt
        })
    }
}

$orderedOccurrences = @($occurrences | Sort-Object path, line, column)
$classificationSummary = [ordered]@{}
foreach ($group in ($orderedOccurrences | Group-Object classification | Sort-Object Name)) {
    $classificationSummary[$group.Name] = $group.Count
}
$ruleSummary = [ordered]@{}
foreach ($group in ($orderedOccurrences | Group-Object rule | Sort-Object Name)) {
    $key = if ([string]::IsNullOrWhiteSpace($group.Name)) { 'UNCLASSIFIED' } else { $group.Name }
    $ruleSummary[$key] = $group.Count
}

$report = [ordered]@{
    schemaName = 'drs.presentationIdentityOccurrenceLedger'
    schemaVersion = 2
    generatedAtUtc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
    scope = if ($RepositoryOnly) { 'repository' } else { 'workspace' }
    workspaceRoot = $workspaceRoot
    legacyIdentity = $rules.legacyIdentity
    approvedIdentity = $rules.approvedIdentity
    occurrenceCount = $orderedOccurrences.Count
    classificationSummary = $classificationSummary
    ruleSummary = $ruleSummary
    occurrences = $orderedOccurrences
}

$unclassifiedCount = @($orderedOccurrences | Where-Object classification -eq 'UNCLASSIFIED').Count
if (-not $NoWrite) {
    $resolvedOutput = [IO.Path]::GetFullPath($OutputPath)
    $outputDirectory = Split-Path $resolvedOutput -Parent
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    $reportJson = ($report | ConvertTo-Json -Depth 10) + "`n"
    [IO.File]::WriteAllText($resolvedOutput, $reportJson, [Text.UTF8Encoding]::new($false))
    Write-Host "Identity ledger: $resolvedOutput"
}

Write-Host "Identity occurrences: $($orderedOccurrences.Count)"
foreach ($entry in $classificationSummary.GetEnumerator()) {
    Write-Host ("  {0}: {1}" -f $entry.Key, $entry.Value)
}

if ($unclassifiedCount -ne 0) {
    Write-Error "$unclassifiedCount identity occurrence(s) are unclassified."
    exit 1
}

Write-Host 'Identity audit classification passed: zero unclassified occurrences.'
