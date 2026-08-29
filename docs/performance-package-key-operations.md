# Performance Package Key Operations

Status: Controlled-service profile retained for future licensed publishing;
portable offline profile is specified in
`performance-package-offline-portable-protection-plan.html`

Date: 2026-08-28

## Security boundary for the controlled-service profile

This runbook describes the stronger, future controlled-service profile.
Portable offline packages intentionally use a separate application-contained
protection profile; that profile does not claim publisher identity and is not
covered by the “private key never ships” boundary below.

Controlled-service desktop targets contain public publisher verification keys
only. Symmetric release keys are resolved at runtime through
`PackageReleaseKeySource`; publisher private keys are accepted only by the
opt-in `drs_controlled_package_signer` CI/service executable through raw
standard input. Neither secret is a command-line argument, CMake value,
generated header, log field, crash field, or package field.

The signing executable is disabled by default with
`DRS_BUILD_CONTROLLED_PACKAGE_SIGNER=OFF` and is not linked by the plugin,
standalone application, or engine adapter.

## Key classes

| Key | Purpose | Permitted location | Rotation |
| --- | --- | --- | --- |
| Package content key | Encrypts one package's records | Export process and active package generation | Every export |
| Release encryption key | Wraps package content keys | Managed release/entitlement system and approved runtime source | Planned release rotation |
| Publisher signing private key | Signs canonical production package bytes | Controlled signer infrastructure only | Planned rotation or compromise |
| Publisher signing public key | Verifies package origin | Read-only desktop trust store | Trust-store update |

## Operational rules

1. No production private key or release key is committed to the repository,
   fixtures, source comments, logs, symbols, crash reports, or package bytes.
2. Development/test keys use separate IDs and cannot be accepted by a release
   trust store.
3. Key lookup is exact by immutable key ID; package metadata never derives a
   key.
4. Content keys use the approved cryptographic RNG and are retired when the
   package operation ends.
5. Backups and recovery remain outside application source and require audited,
   least-privilege release ownership.

## Required production ownership

Before CPCA-3 can pass its operational exit gate, release engineering and
security must record named owners for:

- publisher signing service and hardware/managed key storage;
- release-key entitlement service or OS-protected storage;
- public trust-store changes and emergency revocation;
- signing audit retention and incident response;
- recovery exercises and release approval.

No production private or symmetric key is generated or stored by this
repository.

## Portable offline profile generation

The portable profile is generated outside the repository from a managed raw
32-byte secret file. The generator creates a fresh random mask and writes only
XOR fragments plus non-secret profile/key lifecycle metadata to the generated
header. The source secret and generated header are build inputs, not checked-in
files.

```text
pwsh -NoProfile -File tools/generate-offline-package-profile.ps1 \
  -SecretFile <managed-secret-path> \
  -OutputHeader <private-build-path>/PackageOfflineProtection.generated.h \
  -ProfileId practical-sampler.offline.v1 \
  -ReleaseKeyId ps-offline-release-2026-01 \
  -SigningSecretFile <managed-ed25519-private-key-path> \
  -SigningKeyId ps-recognition-signing-2026-01

cmake -S . -B build/release \
  -DDRS_OFFLINE_PACKAGE_PROFILE_HEADER=<private-build-path>/PackageOfflineProtection.generated.h
```

The secret path must not be echoed into ordinary logs, and the generated
header must not be archived as a source artifact. To retain historical package
readability during a planned rotation, pass retired or revoked slot specs as
`keyId|secretFile|activatedUtc|retiredUtc` or
`keyId|secretFile|activatedUtc|revokedUtc`. The application profile remains
recoverable by a determined reverse engineer; this mechanism is deliberate
friction, not a hardware-backed secret boundary. Profile and key IDs containing
test/development tokens are rejected by the generator and runtime.
The optional signing secret is a 64-byte Ed25519 private-key representation;
its matching public key must be supplied separately through the public-only
recognition trust-store configuration.

## Recognition trust-store provisioning

Configure `DRS_PACKAGE_PUBLISHER_TRUST_ENTRIES` at desktop build time. The
historical variable name is retained for wire/build compatibility; its entries
are a public-only recognition allow-list, not a list of trusted authors. It is a
semicolon-separated list of entries:

```text
keyId|64-hex-character-public-key|active|activatedUtc|retiredUtc|revokedUtc
```

State is exactly `active`, `retired`, or `revoked`. Empty retirement and
revocation timestamps are permitted only when they do not apply. Multiple
public keys support rotation. Retired keys continue to verify historical
packages; revoked keys fail closed. Duplicate IDs, wrong key lengths, and
missing lifecycle metadata invalidate the complete store.

The older single-key public CMake fields remain a compatibility input when the
multi-entry value is empty. They must not be used for private or release-key
material. A matching private recognition key is consumed only by the local
offline signer and is never placed in this trust-store header.

## Future controlled signing (licensed profile)

Build the isolated tool only in the controlled signing environment:

```text
cmake -S . -B build/signing -DDRS_BUILD_CONTROLLED_PACKAGE_SIGNER=ON
cmake --build build/signing --target drs_controlled_package_signer
```

Invocation contract:

```text
secret-manager-command |
  drs_controlled_package_signer <key-id> <canonical-signed-region> \
    <detached-signature-output> <audit-json-output>
```

The secret-manager command must emit exactly 64 raw private-key bytes and
close standard input. The signer validates canonical V3 framing and key ID,
creates an Ed25519ph signature, and requires the audit event to be written
before returning the signature. Audit data contains key ID, SHA-256 digest,
signed byte count, event time, and audit ID—never private, release, content, or
plaintext bytes.

CI must run signing on an isolated identity with no desktop-build credentials.
Desktop build identities must not have permission to invoke the production
signing job or read its key.

## Release-key resolution

The portable profile supplies `PackageReleaseKeySource` from the application-
contained offline fragments. `VersionedPackageKeyProvider` applies the
compiled/configured lifecycle policy:

- active: may encrypt new packages and decrypt existing packages;
- retired: may decrypt existing packages only;
- revoked: may not encrypt or decrypt;
- unknown, unavailable, duplicate, or wrong-length: fail closed with no key.

Source-specific errors must not flow to UI, logs, or package diagnostics. An
entitlement service or OS-protected store is reserved for a future licensed
profile.

## Desktop export injection

The desktop and plug-in shells contain no raw private key or raw release key.
Phase 3 will supply a valid
`PerformancePackageExportSecurityContext` to
`PerformancePackageExportService::setSecurityContext` before export. The
context names the active encryption/signing key IDs and owns references to the
offline `PackageKeyProvider`, local
`PackagePublisherSigningClient`, and public-only
`PackagePublisherTrustStore`.

The normal asynchronous Export Playable Instrument request inherits that
context from the service. The synchronous processor export path obtains the
same service context. If provisioning is absent, invalid, retired, revoked, or
unavailable, export reports a redacted security-configuration failure and
publishes no V1, V2, partial V3, or staging file.

## Rotation procedure

1. Generate the new recognition public/private pair and release key under new,
   immutable IDs, outside source control.
2. Add the new recognition public key as active while retaining the old public
   key as retired.
3. Mark the old release key retired; keep decryption available during the
   supported package lifetime.
4. Publish and reopen a canary package using the new IDs.
5. Demonstrate that historical packages still open and new exports cannot
   select retired keys.
6. Record audit evidence and approve the cutover.

## Revocation and recovery

Revocation is an explicit security response, not routine retirement. Marking a
publisher key revoked causes packages signed by it to fail authentication.
Marking a release key revoked prevents its content key from being unwrapped.
The incident owner must document customer impact before distribution.

Incident response must disable the affected key for new publishing, preserve
forensic evidence without collecting plaintext or keys in ordinary logs,
provision replacement IDs, update the trust/key registry, re-export affected
packages where necessary, publish compatibility guidance, and archive review
sign-off.

Quarterly recovery evidence must cover signing-service outage, release-key
source outage, new-key activation, old-key retirement, public-key revocation,
audit-sink outage, and restoration from managed backups. Every failure must
return no key/signature/plaintext and must not alter an already active playback
generation.
