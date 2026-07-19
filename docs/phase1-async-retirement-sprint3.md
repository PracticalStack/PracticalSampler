# Phase 1 Prepared Playback Async Retirement Note

This note captures Sprint 3 task `S3.6-T1` from section 6.1 of `engineering-plan.html`: make prepared-asset retirement explicitly asynchronous and prove the heavy cleanup path stays off the audio thread.

## What changed

- `PreparedPlaybackService` now exposes `serviceRetiredCacheCleanup(...)` as the named non-audio cleanup seam for retired prepared cache entries.
- `EngineFacade::serviceBackgroundWork()` now services one retired prepared cache entry before applying newly completed worker results.
- That ordering preserves an explicit retirement backlog for freshly invalidated prepared assets until a later shell/message-thread service pass drains it.

## Why the ordering matters

- Worker completion can retire stale prepared ownership when a newly built cache key supersedes an older sample handle.
- If cleanup runs in the same service pass that applies the new result, the backlog disappears immediately and the product cannot prove the retirement policy.
- By draining previously retired entries first, then applying new completions, the system keeps a visible backlog until the next non-audio service pass.

## Regression coverage

- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - verifies retired prepared ownership remains queued until explicit non-audio cleanup is serviced
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - verifies facade invalidating-preview integration takes the expected one-hit/one-miss prepared path while diagnostics stay aligned
- `tests/src/Phase1RealtimeSafetyTests.cpp`
  - verifies invalidating prepared-preview servicing stays off the audio thread and does not increment `largeResourceReleasesOnAudioThread`

## Current boundary

- audio thread:
  - never destroys retired prepared cache entries
- worker thread:
  - builds new prepared assets and records superseded ownership for later cleanup
- shell/message-thread service loop:
  - performs the actual retired prepared cleanup work
