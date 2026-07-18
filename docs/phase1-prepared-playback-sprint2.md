# Phase 1 Prepared Playback Handles

This note captures the fourth Sprint 2 slice for section 6.1 of `engineering-plan.html`: converting a valid immutable playback snapshot into a product-owned prepared playback result with immutable sample and stream handles.

## What changed

- added `PreparedPlaybackService` as the first product-owned preparation seam after immutable snapshot construction
- added immutable prepared playback contracts for:
  - prepared sample handles
  - prepared stream handles
  - prepared zone bindings
  - preparation metrics and deterministic prepared-content digests
- keyed prepared handles by canonical source path, source fingerprint, payload encoding, page size, and compiler version so unchanged content can warm-hit deterministically
- threaded prepared build identity, digest, asset counts, byte counts, and cache metrics into `DraftPlaybackContract` and `EngineFacade`

## Why this matters

The earlier Sprint 2 slices proved that Preview and Publish can build and report immutable snapshots, but they still stopped before prepared asset ownership.

This step moves the pipeline one stage closer to the architecture in section 6.1:

- Preview and Publish now retain immutable prepared asset metadata instead of only snapshot metadata
- sample and stream ownership now have explicit product-owned handles instead of implicit dependence on the shell runtime fixture
- preparation can evolve into a worker-owned async service later without redefining the contract again

## Validation

The focused regression slice now proves that:

- repeated preparation of the same snapshot produces the same prepared digest
- the first build cold-misses the cache while a repeated build warm-hits unchanged prepared handles
- changing one source path invalidates exactly the affected prepared asset key
- facade and diagnostics snapshots expose prepared build ids, digests, and prepared asset counts

This is still a Sprint 2 boundary. The new prepared result is deterministic and product-owned, but it is still built synchronously and does not yet perform renderer-side activation or asynchronous worker orchestration.
