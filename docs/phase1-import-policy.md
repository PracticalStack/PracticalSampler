# Phase 1 Import Policy

This note captures the Sprint 2 policy slice for imported sample content. The import seam now distinguishes between:

- decode failures, where the source file could not be read at all
- policy warnings, where the file is usable but needs review
- policy errors, where the file decodes but must not become a Phase 1 runtime artifact

## Hard acceptance rules

Phase 1 accepts a source sample only when all of the following are true:

- the decoded source format is WAV or FLAC
- the decoded sample rate is `44100` Hz or `48000` Hz
- the decoded channel count is `1` or `2`
- when compiling, the source path resolves under the configured content root
- when compiling, the source path resolves under the content root `Samples/` directory

If any of those rules fail, the importer or compiler must emit an actionable error and refuse to produce a successful Phase 1 artifact.

## Warning-only rules

Phase 1 currently treats naming portability as a warning, not a hard error.

The importer warns when a sample filename stem uses characters outside:

- letters
- digits
- underscore
- hyphen

This keeps the runtime path flexible while still flagging names that are likely to cause trouble in future tooling, packaging, or cross-platform content moves.

## Validation

`drs_phase1_sample_import_tests` now covers:

- supported WAV and FLAC happy paths
- warning-only naming-policy findings
- hard rejection of AIFF content even though JUCE can decode it
- hard rejection of unsupported sample rates
- hard rejection of unsupported channel counts

`drs_phase1_compile_path_tests` now covers:

- hard rejection of compile inputs outside the configured content root
- hard rejection of compile plans that carry unsupported sample metadata
- warning propagation for non-portable sample names during compile

## Why this slice matters

Sprint 2 is not done when the importer can decode audio. It is done when the team can trust that unsupported content fails loudly instead of silently entering the runtime corpus and making Sprint 3 streaming bugs harder to diagnose.
