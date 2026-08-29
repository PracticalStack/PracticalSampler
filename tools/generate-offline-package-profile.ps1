[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $SecretFile,

    [Parameter(Mandatory = $true)]
    [string] $OutputHeader,

    [string] $ProfileId = 'practical-sampler.offline.v1',
    [string] $ReleaseKeyId = 'ps-offline-release-2026-01',
    [string] $ActivatedUtc = '2026-09-01T00:00:00Z',
    [string[]] $RetiredKey = @(),
    [string[]] $RevokedKey = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-KeyIdentifier([string] $Value, [string] $Name) {
    if ([string]::IsNullOrWhiteSpace($Value) -or $Value.Length -gt 4096 -or
        $Value -notmatch '^[A-Za-z0-9._-]+$') {
        throw "$Name must contain only ASCII letters, digits, '.', '_', or '-'."
    }
}

function Test-ProductionIdentifier([string] $Value, [string] $Name) {
    $reserved = @('test', 'tests', 'dev', 'debug', 'fixture', 'example', 'development')
    foreach ($token in ($Value -split '[._-]+')) {
        if ($reserved -contains $token.ToLowerInvariant()) {
            throw "$Name must not use a test/development identifier token ('$token')."
        }
    }
}

function Test-Timestamp([string] $Value, [string] $Name, [bool] $Required = $true) {
    if (-not $Required -and [string]::IsNullOrWhiteSpace($Value)) { return }
    if ([string]::IsNullOrWhiteSpace($Value) -or
        $Value -notmatch '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$') {
        throw "$Name must be an RFC3339 UTC timestamp such as 2026-09-01T00:00:00Z."
    }
}

function Format-ByteArray([byte[]] $Bytes) {
    return (($Bytes | ForEach-Object { '0x{0:x2}' -f $_ }) -join ', ')
}

function Clear-ByteArray([byte[]] $Bytes) {
    if ($null -eq $Bytes) { return }
    try {
        [System.Security.Cryptography.CryptographicOperations]::ZeroMemory($Bytes)
    }
    catch [System.Management.Automation.RuntimeException] {
        # Windows PowerShell 5/.NET Framework has no CryptographicOperations.
        # Clear the managed array as the best available fallback; the generated
        # header never contains this array after the function returns.
        [System.Array]::Clear($Bytes, 0, $Bytes.Length)
    }
}

function Read-KeySlot([string] $KeyId, [string] $State, [string] $SlotActivatedUtc,
                      [string] $RetiredUtc, [string] $RevokedUtc, [string] $SlotSecretFile) {
    Test-KeyIdentifier $KeyId 'ReleaseKeyId'
    Test-ProductionIdentifier $KeyId 'ReleaseKeyId'
    Test-Timestamp $SlotActivatedUtc 'ActivatedUtc'
    Test-Timestamp $RetiredUtc 'RetiredUtc' ($State -eq 'retired')
    Test-Timestamp $RevokedUtc 'RevokedUtc' ($State -eq 'revoked')
    if (-not (Test-Path -LiteralPath $SlotSecretFile -PathType Leaf)) {
        throw "Secret file does not exist: $SlotSecretFile"
    }
    $secretBytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $SlotSecretFile))
    $mask = [byte[]]::new(32)
    $xor = [byte[]]::new(32)
    $success = $false
    try {
        if ($secretBytes.Length -ne 32) {
            throw "Secret files must contain exactly 32 raw bytes; received $($secretBytes.Length)."
        }
        $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
        try { $rng.GetBytes($mask) } finally { $rng.Dispose() }
        for ($index = 0; $index -lt 32; $index++) {
            $xor[$index] = $mask[$index] -bxor $secretBytes[$index]
        }
        $success = $true
        return [pscustomobject]@{
            KeyId = $KeyId; State = $State; ActivatedUtc = $SlotActivatedUtc
            RetiredUtc = $RetiredUtc; RevokedUtc = $RevokedUtc; Mask = $mask; Xor = $xor
        }
    }
    finally {
        Clear-ByteArray $secretBytes
        if (-not $success) {
            Clear-ByteArray $mask
            Clear-ByteArray $xor
        }
    }
}

Test-KeyIdentifier $ProfileId 'ProfileId'
Test-ProductionIdentifier $ProfileId 'ProfileId'
Test-KeyIdentifier $ReleaseKeyId 'ReleaseKeyId'
Test-ProductionIdentifier $ReleaseKeyId 'ReleaseKeyId'
Test-Timestamp $ActivatedUtc 'ActivatedUtc'

$slots = [System.Collections.Generic.List[object]]::new()
$slots.Add((Read-KeySlot $ReleaseKeyId 'active' $ActivatedUtc '' '' $SecretFile))

foreach ($spec in $RetiredKey) {
    $parts = $spec -split '\|', 4
    if ($parts.Count -ne 4) {
        throw 'RetiredKey must use keyId|secretFile|activatedUtc|retiredUtc.'
    }
    $slots.Add((Read-KeySlot $parts[0] 'retired' $parts[2] $parts[3] '' $parts[1]))
}
foreach ($spec in $RevokedKey) {
    $parts = $spec -split '\|', 4
    if ($parts.Count -ne 4) {
        throw 'RevokedKey must use keyId|secretFile|activatedUtc|revokedUtc.'
    }
    $slots.Add((Read-KeySlot $parts[0] 'revoked' $parts[2] '' $parts[3] $parts[1]))
}

$keyIds = @{}
$quote = [char]34
$slotText = foreach ($slot in $slots) {
    if ($keyIds.ContainsKey($slot.KeyId)) { throw "Duplicate release key ID: $($slot.KeyId)" }
    $keyIds[$slot.KeyId] = $true
    $retired = if ([string]::IsNullOrEmpty($slot.RetiredUtc)) { 'nullptr' } else { $quote + $slot.RetiredUtc + $quote }
    $revoked = if ([string]::IsNullOrEmpty($slot.RevokedUtc)) { 'nullptr' } else { $quote + $slot.RevokedUtc + $quote }
    "    ReleaseKeySlot{ $quote$($slot.KeyId)$quote, $quote$($slot.State)$quote, $quote$($slot.ActivatedUtc)$quote, $retired, $revoked, std::array<std::uint8_t, 32>{ $(Format-ByteArray $slot.Mask) }, std::array<std::uint8_t, 32>{ $(Format-ByteArray $slot.Xor) } },"
}

$header = @"
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Generated offline package protection profile. Do not commit this file or
// the source secrets. The two arrays in each slot are XOR fragments, not raw
// key bytes. Recovery by deliberate reverse engineering remains possible.
namespace drs::engine::offline_generated
{
struct ReleaseKeySlot
{
    const char* keyId;
    const char* state;
    const char* activatedUtc;
    const char* retiredUtc;
    const char* revokedUtc;
    std::array<std::uint8_t, 32> mask;
    std::array<std::uint8_t, 32> xorFragment;
};

inline constexpr const char* profileId = "$ProfileId";
inline constexpr std::array<ReleaseKeySlot, $($slots.Count)> releaseKeys = {
$($slotText -join [Environment]::NewLine)
};
} // namespace drs::engine::offline_generated
"@

$outputPath = [System.IO.Path]::GetFullPath($OutputHeader)
$outputDirectory = [System.IO.Path]::GetDirectoryName($outputPath)
if (-not [string]::IsNullOrEmpty($outputDirectory)) {
    [System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
}
$utf8 = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($outputPath, $header, $utf8)

foreach ($slot in $slots) {
    Clear-ByteArray $slot.Mask
    Clear-ByteArray $slot.Xor
}
Write-Output "Generated offline package profile header: $outputPath"
