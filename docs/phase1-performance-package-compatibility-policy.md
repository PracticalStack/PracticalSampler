# Phase 1 Performance Package Compatibility Policy

Decision date: August 4, 2026

## Frozen contract

The playable package format is now frozen at:

- `schemaName`: `drs.performancePackage`
- `schemaVersion`: `1`
- `schemaMajorVersion`: `1`
- `schemaMinorVersion`: `0`
- `compatibilityPolicyId`: `drs.performancePackage.policy.v1.0`

The current protected-package reader contract also assumes:

- the sealed package extension remains `.drpkg`
- the required runtime-only payload set remains:
  - `packageManifest`
  - `runtimeInstrument`
  - `runtimeStreamIndex`
  - `runtimeStreamPayload`
- optional authenticated artwork and license payloads may be present
- package sessions never fall back to nearby raw samples or authored project manifests
- new production exports use signed `DRSPKG3` only

## Reader behavior

### Current protected package

V3 readers must load packages only when all of the following are true:

- `schemaName` matches `drs.performancePackage`
- `formatVersion` is supported by the current reader
- `minimumReaderSchemaVersion` is less than or equal to the current reader schema version
- the publisher signature verifies before protected record plaintext is used
- the release-key provider resolves the declared key and the content-key envelope authenticates
- all required records decrypt, validate, and remain runtime-compatible

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

## Disk-format and migration policy

Package V3 (`DRSPKG3`) is the only protected production export format. It uses
bounded, independently authenticated XChaCha20-Poly1305 records, a wrapped
content key, 64-bit offsets, and a publisher signature covering the canonical
package representation.

Package V1 (`DRSPKG1`) and V2 (`DRSPKG2`) are unprotected legacy compatibility
inputs. They remain readable within their existing bounds so customer sessions
continue to open, but the application never rewrites them, upgrades them on
open, or describes them as encrypted, authenticated, sealed, or protected.
V1 remains subject to the 64 MiB resident ceiling. A legacy creator migrates by
opening the original editable project and explicitly exporting a new V3
package; there is no supported in-place conversion when the source project is
unavailable.

Production application targets do not link V1/V2 writer entry points. The
legacy writers and deterministic compatibility crypto are available only to an
explicit test-fixture target used to preserve reader coverage.

See `docs/large-instrument-streaming-support.md` for budgets, lifecycle states, recovery, and qualification status.
