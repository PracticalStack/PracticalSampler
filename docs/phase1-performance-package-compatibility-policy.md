# Phase 1 Performance Package Compatibility Policy

Decision date: August 4, 2026

## Frozen contract

The playable package format is now frozen at:

- `schemaName`: `drs.performancePackage`
- `schemaVersion`: `1`
- `schemaMajorVersion`: `1`
- `schemaMinorVersion`: `0`
- `compatibilityPolicyId`: `drs.performancePackage.policy.v1.0`

The current reader contract also assumes:

- the sealed package extension remains `.drpkg`
- the runtime-only payload set remains exactly:
  - `packageManifest`
  - `runtimeInstrument`
  - `runtimeStreamIndex`
  - `runtimeStreamPayload`
- package sessions never fall back to nearby raw samples or authored project manifests

## Reader behavior

### Current package

Readers must load packages when all of the following are true:

- `schemaName` matches `drs.performancePackage`
- `formatVersion` is supported by the current reader
- `minimumReaderSchemaVersion` is less than or equal to the current reader schema version
- all required payloads decrypt, validate, and remain runtime-compatible

### Future minor changes

Future minor changes are allowed only when they are additive and keep the Phase 1 sealed-package contract intact:

- `schemaName` stays the same
- `formatVersion` stays the same
- existing payload ids, kinds, and authentication rules stay valid
- older readers are still safe to use because `minimumReaderSchemaVersion` does not move above the current reader

If a proposed minor change needs a higher `minimumReaderSchemaVersion`, the current Phase 1 reader rejects that package.

### Future major changes

Future major changes are incompatible by default.

Examples:

- changing payload meaning or required payload inventory
- changing authenticated-data composition
- changing cleartext metadata shape in a breaking way
- changing the sealed file structure or `formatVersion`

Those changes require a new reader contract. The current Phase 1 reader rejects them through either:

- unsupported `formatVersion`, or
- `minimumReaderSchemaVersion` greater than the current reader version

## Support categories

Phase 1 support and QA should classify failures using the package loader’s explicit categories:

- `package-format-failure`
- `decryption-failure`
- `payload-corruption`
- `playback-compatibility-failure`

These categories are part of the frozen support contract for Sprint 8 release gating.

## Large-instrument extension

Package v2 (`DRSPKG2`) is the bounded streaming format. It uses a clear fixed header/TOC and independently authenticated records with 64-bit offsets and identities. Package v1 remains readable only under the 64 MiB resident compatibility ceiling; a larger v1 package is rejected with explicit v2 re-export guidance. Loading never rewrites a package or resolves adjacent raw samples.

See `docs/large-instrument-streaming-support.md` for budgets, lifecycle states, recovery, and qualification status.
