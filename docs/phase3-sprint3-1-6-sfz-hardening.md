# Phase 3.1.6 - SFZ Hardening And Corpus Expansion

Frozen July 21, 2026.

Sprint 3.1.6 turns the first playable SFZ workflow into a regression-catching suite. The focus of
this slice is not new user-facing behavior. It is confidence: broader corpus coverage, repeatable
analysis and projection, and proof that the saved native artifacts remain stable when the importer
is exercised beyond the first mono crossfade fixture.

## What this slice hardens

- Added a broader checked-in corpus matrix across:
  - `jRhodes3d-mono/_jRhodes3d-mono-flac.sfz`
  - `jRhodes3d-mono-no-xfade.sfz`
  - `jRhodes3d-st.sfz`
  - `jRhodes3d-st-no-xfade.sfz`
  - `jRhodes3d-sv.sfz`
  - `jRhodes3d-sv-no-xfade.sfz`
- Added deterministic repeat-run coverage for:
  - full analyze/report output
  - projection-from-analysis vs projection-from-document parity
  - repeated projection stability
- Broadened save/load hardening with a second round-trip fixture:
  - stereo no-crossfade import survives `.drsproj` and `.drinst` persistence with the same
    round-robin and release metadata

## Expected corpus behavior

- The `*-no-xfade` fixtures should keep the review gate because the width-control curve remains
  report-first, but they should no longer emit velocity-crossfade approximation findings.
- The stereo and stereo-vibrato fixtures should preserve the same region and sample-source shape as
  the mono fixture while exercising different corpus roots and sample naming.
- All six checked fixtures should remain playable after projection and should continue to save
  creator-facing provenance notes.

## Tests added

- `drs.sprint31.sfz_corpus_hardening`
- `drs.sprint31.sfz_determinism`

These sit on top of the earlier Sprint 3.1 parser, normalization, compatibility, report-model,
projection, and review-UI slices. Their job is to fail fast when a future parser or conversion
change quietly alters ordering, summary counts, or broadened corpus behavior.
