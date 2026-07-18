# Phase 1 Draft Playback Integration

This note captures the second Sprint 2 slice for section 6.1 of `engineering-plan.html`: wiring immutable playback snapshot results into the existing draft, preview, and publish contract.

## What changed

- `DraftPlaybackContract` prepared revisions now retain:
  - snapshot build identity
  - activation eligibility
  - lifecycle state
  - content digest
  - structured findings
- `EngineFacade::refreshPreviewToCurrentDraft()` now completes against a real immutable snapshot build
- `EngineFacade::publishCurrentDraft()` now completes against a real immutable snapshot build
- failed or ineligible snapshot results preserve the last known-good preview or published revision while surfacing findings

## Why this matters

Sprint 1 froze the user-visible revision behavior, but completion was still synthetic. This step turns that seam into real product logic:

- preview readiness now means a snapshot actually built
- publish activation now means a snapshot actually passed activation-eligibility checks
- stale and failed states now carry structured findings from the snapshot builder instead of ad hoc error strings

## Validation

The updated regression coverage now proves that:

- draft/playback contract tests use real snapshot builds for preview and publish transitions
- facade-level preview and publish transitions carry stable digests for successful builds
- invalid snapshot results do not replace the last known-good prepared or published revision

This is still a Sprint 2 boundary. The snapshot results are real, but they are not yet connected to prepared assets, asynchronous decode, or renderer activation. Those remain for later Sprint 2 and Sprint 3 work.
