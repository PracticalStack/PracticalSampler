# Phase 1 Prepared Playback Retirement Regression Note

This note captures Sprint 3 task `S3.6-T4` from section 6.1 of `engineering-plan.html`: add regression coverage for queued cancellation cleanup, retirement backlog growth, and post-retirement cache reuse behavior.

## What changed

- `Phase1PreparedPlaybackWorkerTests.cpp` now covers queued preview and publish cancellation on the background-worker path and asserts that canceled work leaves no completed results, active ownership, or retired backlog behind.
- The same worker suite now performs repeated invalidating edits before cleanup and verifies that retired ownership records and retained bytes accumulate until the non-audio cleanup seam drains them.
- After draining retired ownership, the worker suite rebuilds a once-retired cache key and proves it cold-misses instead of silently reusing cleaned retired state.

## Why this matters

- Queue cancellation should be observable cleanup, not a hidden partial build that leaks ownership or completion state.
- Repeated invalidations are the realistic stress case for edited drafts, especially while a user is iterating quickly before the shell gets a chance to service retirement.
- Once retired ownership is cleaned, the cache contract must rebuild that content deterministically rather than resurrecting stale state from an already-retired handle.

## Regression coverage

- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - verifies queued preview and publish cancellation leave the worker idle with no orphaned completions or ownership state
  - verifies repeated invalidations grow the retired backlog across multiple edits and support partial plus full cleanup
  - verifies a cleaned retired cache key rebuilds as a cold miss while the still-active sibling handle remains warm
