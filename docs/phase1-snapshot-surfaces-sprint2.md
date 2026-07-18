# Phase 1 Snapshot Status Surfaces

This note captures the third Sprint 2 slice for section 6.1 of `engineering-plan.html`: surfacing immutable playback snapshot identity, digest, and findings through the existing shell-facing status and diagnostics seams.

## What changed

- `EnginePerformanceSnapshot` now carries preview and published snapshot metadata:
  - build identity
  - activation eligibility
  - content digest
  - structured findings
- `EngineDiagnosticsSnapshot` now mirrors the same snapshot metadata so the diagnostics surface can report the actual state of the draft-to-playback pipeline
- `EngineFacade::getStatusSnapshot()` now summarizes preview and published snapshot ids, digests, and finding counts in the detail text
- the shared `PerformancePanel` and `StatusPanel` now expose those snapshot fields directly instead of only showing high-level revision state

## Gate A frozen shell contract

For Gate A, the minimum Sprint 2 shell-facing draft-playback contract is now intentionally frozen around these field groups:

- Revision identity:
  - draft revision
  - preview revision
  - published revision
- Lifecycle state:
  - preview revision state
  - published revision state
  - pending preview/publish flags where relevant to the surface
- Activation eligibility:
  - preview activation eligibility
  - published activation eligibility
- Immutable snapshot identity:
  - preview snapshot build id
  - published snapshot build id
  - preview snapshot digest
  - published snapshot digest
- Prepared-playback identity:
  - preview prepared build id
  - published prepared build id
  - preview prepared digest
  - published prepared digest
- Prepared asset counts:
  - preview prepared sample/stream/zone counts
  - published prepared sample/stream/zone counts
  - preview/published prepared byte counts
- Findings:
  - preview findings
  - published findings
  - last draft-playback event text
- Playable-range summary:
  - whether a prepared playable range is available
  - lowest playable note
  - highest playable note
  - whether the range comes from published content, preview fallback, or default fallback
- Provenance:
  - which shell surface state is being shown: `published draft`, `preview fallback`, or `default fallback`
  - whether the current renderer path is still `reference-backed` or `inactive`

This freeze is about the shell contract only. It does not claim that the real-time renderer already consumes the authored snapshot directly.

## Explicit non-goals

The following remain outside the Sprint 2 contract freeze and must continue to land in later sprints:

- worker-owned decode and cache lifetime policy
- shared authored sampler core
- authored-performance render-path cutover
- fixture-free production playback
- final voice-transition policy during publish activation

## Why this matters

The first two Sprint 2 slices made snapshot construction and contract wiring real, but contributors still had to infer whether preview or publish readiness came from an actual snapshot build.

This step closes that gap:

- the shell can now show which snapshot build is currently prepared or published
- digest visibility makes it clear when preview and publish point at the same frozen content
- structured findings move from hidden engine state into the visible debugging path

## Validation

The focused regression slice now proves that:

- facade snapshots surface non-empty preview and publish build ids and digests for successful preview and publish flows
- diagnostics snapshots preserve snapshot identity and digest information across session restores
- the shared status detail text reports snapshot ids, digests, playable-range summaries, and provenance
- the existing structured findings remain available through diagnostics when snapshot preparation fails

This is still a Sprint 2 bridge step. The shell now reports real snapshot state, but asset preparation, async decode, and renderer-side activation remain later pipeline work.
