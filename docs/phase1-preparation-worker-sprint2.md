# Phase 1 Preparation Worker Queue

This note captures the fifth and sixth Sprint 2 slices for section 6.1 of `engineering-plan.html`: first adding a worker-owned preparation queue with request coalescing, cancellation, priority, and retirement metrics, then moving prepared-playback execution onto a dedicated background worker with facade-owned completion handoff.

## What changed

- extended `PreparedPlaybackService` with explicit worker-queue contracts for:
  - Preview and Publish lanes
  - queue submission
  - supersede/coalescing of older queued work in the same lane
  - cancellation before worker execution
  - priority so Publish work runs ahead of Preview work
- added worker status metrics for:
  - pending work
  - cancellations
  - superseded jobs
  - failures
  - peak pending depth
  - retired bytes awaiting cleanup
- routed the current synchronous `EngineFacade` preparation path through that queue so the worker seam is exercised even before background threads are introduced
- surfaced worker status in shell-facing diagnostics and status detail text
- added an optional background worker thread inside `PreparedPlaybackService` that:
  - waits for queued work plus a loaded stream container
  - executes queued preview and publish jobs off the facade call stack
  - retains completed build results until the facade drains and applies them
- taught `EngineFacade` to:
  - enqueue prepared-preview and prepared-publish requests asynchronously
  - remember the contract request id that each prepared build completion must satisfy
  - expose an explicit `serviceBackgroundWork()` seam so the message thread can consume completed worker results intentionally
  - apply successful completions back into `DraftPlaybackContract` and ignore stale completions after reopen, close, or restart transitions
- preserved current bootstrap behavior by waiting for worker idle during initial contract seeding, so default and restored sessions still surface ready prepared revisions immediately to the shell and tests
- added a lightweight facade state-revision token so shell surfaces can refresh only after real state changes instead of polling full snapshots for side effects

## Why this matters

The previous Sprint 2 step established immutable prepared handles, but the path was still a direct function call.

These steps move the architecture closer to the section 6.1 target:

- preparation now has an explicit worker-owned queue boundary instead of implicit call-order coupling
- newer Preview requests can supersede stale queued Preview work
- Publish work can take priority without redefining prepared-content contracts
- invalidated prepared cache entries now leave an explicit retirement backlog instead of disappearing silently
- prepared-playback execution now happens on a dedicated worker thread instead of being synchronously drained by `EngineFacade`
- contract completion is now an explicit handoff boundary owned by the shell/message thread rather than hidden inside snapshot getters
- standalone and plug-in shells now service background completion work deliberately and let performance/diagnostics panels observe a revision token instead of mutating engine state during reads

## Validation

The focused regression slice now proves that:

- queued Preview work is superseded cleanly by a newer Preview request
- queued Publish work runs before queued Preview work
- canceling queued Preview work leaves no orphaned pending jobs
- source invalidation retires the stale prepared cache entry and exposes bytes awaiting cleanup until retirement drains
- facade, diagnostics, and status surfaces retain green behavior while preparation flows through the worker queue
- preview and publish requests settle cleanly through the background worker before contract state flips to ready or active
- completed worker results do not change preview or publish contract state until explicit message-thread servicing runs
- default bootstrap, diagnostics, and smoke coverage still finish green with the new async boundary in place

Validated with:

- `drs.phase1.prepared_playback`
- `drs.phase1.prepared_playback_worker`
- `drs.phase1.draft_playback_facade`
- `drs.phase1.diagnostics`
- `drs.phase0.smoke`

This is still a Sprint 2 bridge. The background worker and completion handoff are now real and product-owned, but bootstrap still waits for worker idle so the existing shell remains deterministic while we finish the later section 6.1 UI and transport seams.
