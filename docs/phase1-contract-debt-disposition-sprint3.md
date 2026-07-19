# Phase 1 Sprint 3 Contract Debt Disposition

This note captures Sprint 3 task `S3.7-T5` from section 6.1 of `engineering-plan.html`: close or explicitly defer the remaining Sprint 2 contract debt around public mutability and lifecycle states before Sprint 4 builds on the prepared-playback seam.

## Closed now

- Publish requests now use the `activating` lifecycle while work is pending, instead of collapsing every in-flight path into `preparing`.
- Published revisions now surface the `active` lifecycle once the prepared build is successfully applied through the draft-playback contract.
- Failed newer preview or publish attempts no longer leave the visible lifecycle in a misleading failed state when an older good preview or published revision is still the one actually in effect.
- Negative-path migrated-project coverage still surfaces `failed` when there is no older good revision to preserve, so failure versus fallback is now explicit in the contract.

## Explicitly deferred

- `ImmutablePlaybackSnapshot` and `ImmutablePreparedPlayback` remain public aggregates for Sprint 3 and early Sprint 4 compatibility with builder code, facade plumbing, and existing regression coverage.
- Header comments now mark both types as write-once build products that callers must treat as frozen after construction.
- Full encapsulation of those types behind private storage plus const-view accessors is intentionally deferred until the shared-renderer read API stabilizes, because doing it earlier would create a large signature churn across builders, tests, and facade seams without yet knowing the final renderer-facing read contract.

## Why this disposition is acceptable

- lifecycle semantics are now trustworthy enough for Sprint 4 activation work to build on
- the remaining mutability debt is documented as a compatibility compromise rather than silently masquerading as true type-level immutability
- the deferred refactor is now isolated to API hardening rather than correctness of the current preview/publish activation flow

## Verification

Validated on Sunday, July 19, 2026 with:

- `cmake --build --preset build-debug --target drs_phase1_draft_playback_contract_tests drs_phase1_draft_playback_facade_tests drs_phase1_realtime_safety_tests`
- `ctest --preset test-debug -R "drs.phase1.draft_playback_contract|drs.phase1.draft_playback_facade|drs.phase1.realtime_safety" --output-on-failure`
