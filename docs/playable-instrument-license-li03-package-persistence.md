# Playable Instrument License LI-03 Package Persistence

Status: complete on 2026-08-14. Activation ownership and viewing begin in LI-04.

## Delivered behavior

- Export resolves optional `<content-root>/LICENSE.txt` and revalidates it at the
  package boundary, protecting against edits made after project import.
- Licensed manifests serialize and parse `license.payloadId` with the canonical
  value `license-text`.
- Conventional packages seal a `licenseText` payload using logical path
  `LICENSE.txt` and media type `text/plain; charset=utf-8`.
- Package-v2 exports split license bytes into authenticated records no larger
  than 64 KiB and reconstruct them in contiguous page order.
- Both readers return the authenticated, byte-identical payload through
  `PerformancePackageLoadResult::licenseText`.
- Projects and packages without a license retain their existing behavior and
  package schema versions.

## Trust boundary

`validatePlayableInstrumentLicenseBytes` is the common policy used by project
import, export, and package readers. It enforces the 1 MiB ceiling, strict UTF-8,
embedded-NUL rejection, and binary-control rejection while accepting a UTF-8
BOM and preserving accepted bytes exactly.

Conventional writing rejects missing references, duplicate or wrong-kind
license payloads, noncanonical media/path metadata, and invalid bytes. The
conventional reader rechecks authenticated payload metadata and bytes.
Package-v2 loading requires the declared `licenseText` record sequence, applies
bounded accumulation before allocation growth, authenticates every chunk, and
then validates the reconstructed text. Declared missing, wrong-kind,
noncontiguous, oversized, invalid-UTF-8, or tampered license data fails package
opening before activation.

## Compatibility

The optional license member does not change package or runtime instrument schema
versions. A missing member means no license. Existing schema-1/schema-2 package
fixtures continue to load, and package activation remains unchanged in this
slice. LI-03 intentionally does not retain license ownership in `EngineFacade`
or expose it to either UI shell.

## Evidence

- `drs.playable_instrument_license.contract` passes.
- `drs.playable_instrument_license.project_import` passes after validator reuse.
- `drs.phase1.performance_package` passes with conventional round-trip and
  malformed writer/manifest coverage.
- `drs.performance_package.export_lifecycle` passes with two-record exact-byte
  round trip, deterministic accounting, no-license export, cancellation cleanup,
  missing/wrong-kind/oversized/invalid payload rejection, and sealed-byte
  authentication tampering.
- `drs.package_v2.records`, package loader, package session, and phase-0 smoke
  tests pass.
- Debug VST3 and standalone application targets build successfully.
- LI-01 seams `package-export-persistence` and `package-reader-integrity` now
  return exit code 0 and are backed by registered behavioral coverage.

