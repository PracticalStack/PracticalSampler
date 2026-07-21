# Sprint 3.1.1 SFZ Contract Red Tests

Direct-only target:

- `drs_sprint31_sfz_contract_red_tests`

Status on July 21, 2026: expected-red by design.

## Purpose

Sprint 3.1.1 freezes the SFZ workflow contract before the parser lands. The red audit keeps the
remaining implementation gaps explicit so later slices can retire them deliberately instead of
silently.

## Gaps recorded by the audit

The audit currently expects these seams to remain open:

- no shared `app/src/shared/SfzImportReportModel.h`
- no standalone-shell `.sfz` chooser or review entry path
- no plug-in-shell `.sfz` chooser or review entry path
- no explicit engine-facade SFZ document import entry point

Sprint 3.1.2 retires the parser-only gaps by landing:

- `engine_adapter/include/drs/engine/SfzImport.h`
- `engine_adapter/src/SfzImport.cpp`

If the audit ever goes green unexpectedly, either:

1. a real slice retired one or more seams and the audit should be updated, or
2. the future implementation used a different integration shape and the audit needs to be revised
   intentionally.

## Why this is direct-only

The green contract and fixture-characterization targets are registered with CTest because they
describe current required behavior. The red audit is not registered because it encodes known
missing Phase 3 implementation seams that would make the ordinary test suite fail for the wrong
reason.
