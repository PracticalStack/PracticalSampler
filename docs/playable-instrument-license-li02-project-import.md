# Playable Instrument License LI-02 Project Import

Status: complete on 2026-08-14. Package persistence begins in LI-03.

## Delivered behavior

- Plugin and standalone authoring File menus include `Import License File...`.
- Performance-only sessions do not expose the command because it remains inside
  the existing `authoringAvailable` menu gate.
- An unsaved authoring project enters the existing Save As workflow and resumes
  license import only after a successful save.
- The chooser accepts one `*.txt` file and opens at the saved instrument root.
- Replacing an existing `LICENSE.txt` requires explicit confirmation in both UI
  shells.
- Shared storage writes the accepted bytes to `<project-root>/LICENSE.txt` using
  a staged `juce::TemporaryFile` replacement.

## Validation and safety

`importProjectLicenseFile` rejects a missing source, a non-`.txt` extension,
content larger than 1 MiB, malformed UTF-8, and embedded NUL bytes. A UTF-8 BOM
is accepted; binary C0/DEL controls are rejected except for tab, CR, and LF.
Extension matching is case-insensitive, and all accepted bytes are preserved
exactly. Selecting the canonical destination itself is a validated successful
no-op.

Validation is completed before staging. The staged file is committed only after
the optional pre-commit checkpoint, allowing deterministic regression coverage
of interrupted replacement. Validation, staging, or commit failure returns an
actionable error without deliberately deleting the existing license first.

## Evidence

- `drs.playable_instrument_license.project_import` passes.
- `drs.playable_instrument_license.contract` passes.
- `drs.phase0.smoke` passes.
- The direct LI-01 `project-import-storage` seam now returns exit code 0 and is
  backed by the registered behavioral import test.
- Debug VST3 and standalone application targets build successfully.
- Package export, package reading, activation ownership, and Performance viewing
  remain unchanged for LI-03 and LI-04.
