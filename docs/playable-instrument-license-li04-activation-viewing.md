# Playable Instrument License LI-04 Activation and Viewing

Status: complete on 2026-08-14. This closes the playable-instrument license iteration.

## Delivered behavior

- Conventional and prepared package activation retain authenticated license text as an immutable package-owned value in `EngineFacade` only after all activation preparation succeeds.
- A UTF-8 BOM remains part of the authenticated package bytes but is omitted from display text. Empty valid license text remains distinguishable from a package without a license.
- Failed replacement preserves the active package license. Successful replacement by an unlicensed package, package close, bundled-runtime restore, authoring replacement, and full session reset clear it.
- Plugin and standalone File menus use one truth-table policy and show `View License` only for an active performance-package session with retained text.
- Both commands open the same in-memory, read-only viewer. The viewer never reopens the project or package from disk.
- The license text is selectable, multiline, vertically scrollable, and accompanied by Close and Escape actions plus accessible titles/descriptions. Compact host-window sizing remains usable.

## Ownership boundary

The package reader remains the trust boundary for size, encoding, and authentication. LI-04 does not validate or reload a source file. It converts the already-authenticated payload into immutable display text before the successful state swap, then publishes that pointer with the rest of the package generation.

## Evidence

- `drs.phase1.performance_package` passes conventional activation ownership, BOM display handling, and close cleanup.
- `drs.performance_package.export_lifecycle` passes prepared activation, licensed replacement, failed-replacement preservation, unlicensed replacement cleanup, and bundled restore cleanup.
- `drs.playable_instrument_license.viewer` passes read-only multiline rendering, exact text, selection, accessibility metadata, Close action, and compact layout.
- `drs.playable_instrument_license.contract` passes the menu-visibility truth table; project import remains green.
- LI-01 direct seams `activation-ownership`, `plugin-menu-viewer`, and `standalone-menu-viewer` now return exit code 0.
- Package-session integration, release gate, phase-0 smoke, and realtime-safety tests pass.
- Debug standalone application and VST3 production bundle targets build successfully.
