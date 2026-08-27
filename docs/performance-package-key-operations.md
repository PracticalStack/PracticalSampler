# Performance Package Key Operations Runbook

Status: Accepted for implementation; operational provisioning pending Phase 3

Date: 2026-08-27

## Key classes

| Key | Purpose | Where it may exist | Rotation |
| --- | --- | --- | --- |
| Package content key | Encrypts one package's records | Export process and active package generation | Every export |
| Release encryption key | Wraps package content keys | Controlled release/provisioning system and approved runtime key provider | Planned release rotation |
| Publisher signing private key | Signs production package bytes | Offline/controlled publisher infrastructure only | On compromise or planned rotation |
| Publisher signing public key | Verifies package origin | Application trust store | Ships with trust-store update |

## Rules

1. No production private key or release key is committed to the repository,
   fixtures, source comments, logs, symbols, crash reports, or package bytes.
2. Development and test keys are separate from production key IDs and cannot
   be accepted by a release build.
3. Key lookup is explicit by key ID. Unknown, retired, or unavailable keys
   fail closed with a non-sensitive diagnostic.
4. Content keys are generated with the approved cryptographic RNG, used only
   for the package generation, and cleared when no longer needed.
5. Key backups and recovery are controlled outside the application source;
   access is audited and limited to release owners.
6. A suspected release-key compromise triggers a new key ID, package re-export
   policy, and a documented minimum reader update. A signing-key compromise
   additionally requires trust-store revocation/rotation.

## Initial delivery model

Phase 2 may use an in-memory test provider and a non-production development
key. Production V3 packages are signed by Practical Sampler release
infrastructure. Desktop authoring exports remain development/unsigned
artifacts unless they pass through that controlled publishing path. If
third-party author publishing is introduced later, it must use hosted vendor
signing or an approved certificate chain; a private signing key may not be
embedded in a general desktop export binary.

## Incident response

1. Disable the affected key ID for new publishing.
2. Preserve forensic evidence without collecting plaintext content or keys in
   ordinary logs.
3. Generate a replacement key pair and update the trust/key registry.
4. Re-export affected packages and publish migration guidance.
5. Release the minimum reader update required by the compatibility policy.
6. Record the incident, affected package/key IDs, customer impact, and review
   sign-off in the release archive.
