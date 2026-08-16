param(
    [Parameter(Mandatory = $true)]
    [string] $BaselineProject,
    [Parameter(Mandatory = $true)]
    [string] $HostState,
    [Parameter(Mandatory = $true)]
    [string] $OutputProject
)

$projectLines = [System.Collections.Generic.List[string]]::new()
Get-Content -LiteralPath $BaselineProject | ForEach-Object { $projectLines.Add($_) }

$vstLine = -1
for ($index = 0; $index -lt $projectLines.Count; ++$index) {
    if ($projectLines[$index] -match '^\s*<VST .*Practical Sampler') {
        $vstLine = $index
        break
    }
}
if ($vstLine -lt 0) {
    throw "The baseline project does not contain Practical Sampler."
}

$payloadStart = $vstLine + 2
$payloadEnd = $payloadStart
while ($payloadEnd -lt $projectLines.Count -and !$projectLines[$payloadEnd].EndsWith('=')) {
    ++$payloadEnd
}
if ($payloadEnd -ge $projectLines.Count) {
    throw "The VST3 component-state payload was not found."
}

$oldPayload = [Convert]::FromBase64String(
    (($projectLines[$payloadStart..$payloadEnd]) -join ''))
if ($oldPayload.Length -lt 24) {
    throw "The VST3 component-state payload is unexpectedly short."
}

$jsonEnd = [Array]::IndexOf($oldPayload, [byte] 0, 8)
if ($jsonEnd -lt 8) {
    throw "The JUCE state terminator was not found."
}

$hostStateText = [IO.File]::ReadAllText(
    (Resolve-Path -LiteralPath $HostState),
    [Text.UTF8Encoding]::new($false))
if (!$hostStateText.EndsWith("`n")) {
    $hostStateText += "`n"
}
$hostStateBytes = [Text.UTF8Encoding]::new($false).GetBytes($hostStateText)
$suffix = $oldPayload[$jsonEnd..($oldPayload.Length - 1)]
$newPayload = [byte[]]::new(8 + $hostStateBytes.Length + $suffix.Length)
[Array]::Copy($oldPayload, 0, $newPayload, 0, 8)
[Array]::Copy($hostStateBytes, 0, $newPayload, 8, $hostStateBytes.Length)
[Array]::Copy($suffix, 0, $newPayload, 8 + $hostStateBytes.Length, $suffix.Length)
[Array]::Copy([BitConverter]::GetBytes([uint32] ($newPayload.Length - 16)), 0, $newPayload, 0, 4)

$encoded = [Convert]::ToBase64String($newPayload)
$wrapped = [System.Collections.Generic.List[string]]::new()
for ($offset = 0; $offset -lt $encoded.Length; $offset += 256) {
    $wrapped.Add($encoded.Substring($offset, [Math]::Min(256, $encoded.Length - $offset)))
}

$projectLines.RemoveRange($payloadStart, $payloadEnd - $payloadStart + 1)
$projectLines.InsertRange($payloadStart, $wrapped)

$headerLine = $projectLines[$vstLine + 1]
$headerIndent = $headerLine.Substring(0, $headerLine.Length - $headerLine.TrimStart().Length)
$header = [Convert]::FromBase64String($headerLine.Trim())
if ($header.Length -lt 36) {
    throw "The REAPER VST3 state header is unexpectedly short."
}
[Array]::Copy([BitConverter]::GetBytes([uint32] $newPayload.Length), 0, $header, 32, 4)
$projectLines[$vstLine + 1] = $headerIndent + [Convert]::ToBase64String($header)

[IO.File]::WriteAllLines(
    [IO.Path]::GetFullPath($OutputProject),
    $projectLines,
    [Text.UTF8Encoding]::new($false))
