# Phase 1 Sprint 3 Prepared-Assets Wrap-Up

This note captures Sprint 3 task `S3.6-T5` from section 6.1 of `engineering-plan.html`: record the current prepared-assets outcome, the expected metrics behavior, the remaining limitations, and the assumptions Sprint 4 should rely on.

## Metrics expectations

- unchanged prepared rebuilds should remain visibly warm:
  - `decodedBytes == 0`
  - cache hit rate stays at or near `1.0`
  - prepared sample-data bytes remain stable for the same authored content
- targeted invalidation should stay narrow:
  - one changed source should produce one cold miss while unaffected prepared handles stay warm
  - retired ownership bytes should become visible until non-audio cleanup drains them
- queued cancellation should be observable but not destructive:
  - cancellation counters may increase
  - active ownership bytes and retired ownership backlog should not grow from work that never executed
- cache pressure should remain explainable through the shell-facing two-working-set model:
  - `Idle` when no prepared ownership is resident
  - `Nominal` within one working set
  - `Replacement set retained` while active plus retired ownership spans replacement churn
  - `Over budget` only when resident ownership exceeds the current two-working-set budget

## Known limitations

- Sprint 3 does not replace the current playback renderer; authored-performance cutover remains Sprint 4 and Sprint 6 work.
- Prepared-cache retirement is explicit and measurable, but activation-slot retirement is still a separate shell-side path rather than a unified product-owned cleanup policy.
- Some shell and authoring helpers still exist outside the prepared worker boundary, especially waveform-preview, root-key restore, and import-adjacent sample loading helpers called out in the Sprint 3 boundary audit.
- The pressure policy is reviewable and surfaced through diagnostics, but Sprint 3 stops short of a full shared-renderer memory and stream-pressure response model.
- Reference-backed performance behavior is still present in the product while the authored shared renderer is being built.

## Sprint 4 handoff assumptions

- `PreparedPlaybackService` is now the intended preparation seam for Preview and Publish work; Sprint 4 should consume prepared handles rather than introducing new direct decode paths.
- The audio callback is allowed to consume installed activations only; it must not become a path-resolution, decode, or large-resource-destruction seam.
- Prepared ownership, retirement tokens, digests, and shell-visible metrics are stable enough to be treated as contract inputs to the shared renderer.
- The facade/service loop remains the non-audio owner for draining retired prepared cache entries until a broader lifetime policy is intentionally redesigned.
- Sprint 4 should treat the current metrics as validation signals:
  - cold versus warm prepared builds
  - cancellation and supersede behavior
  - retirement backlog visibility
  - two-working-set pressure state

## Ready signal

Sprint 3 leaves prepared playback in a state where Preview and Publish preparation, cache-key invalidation, retirement backlog visibility, queue behavior, and shell-facing metrics can be extended by Sprint 4 without reopening the prepared-assets contract boundary.
