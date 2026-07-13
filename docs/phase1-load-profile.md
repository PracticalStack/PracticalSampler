# Phase 1 Load Profiles

This note captures the fourth Sprint 3 slice: the first product-owned load-profile policy for Phase 1 runtime playback.

## Current scope

Phase 1 now defines three named profiles:

- `eco` trims both per-voice prefetch and page-cache residency to the smallest reviewed budget.
- `balanced` is the default reference profile for the checked-in corpus.
- `performance` allows larger working sets so active playback is less likely to stall on follow-up page reads.

The current product-owned policy ties each profile to:

- a maximum prefetch budget per allocated voice
- a maximum resident streamed-page budget for the shared background service
- an explicit profile id surfaced through streaming-service metrics

## Downgrade behavior

The first profile-switch path is intentionally conservative.

Switching from a larger profile to a smaller one must:

- preserve any page lease already held by an active voice
- let that voice continue advancing after the downgrade
- defer cache trimming until dormant pages can be evicted safely

`purgeDormantPages()` is the first explicit hook for that last step. It allows Phase 1 tests and CI to prove that dormant content is reviewably purged instead of silently remaining resident forever.

## Validation

`drs_phase1_load_profile_tests` now proves:

- `eco`, `balanced`, and `performance` all resolve by id
- prefetch and resident-page budgets increase predictably across those profiles
- voice allocation clamps prefetch bytes to the selected profile without exceeding the compiled reference head
- downgrading a live service from `performance` to `eco` does not invalidate the active voice lease
- dormant-page purge trims resident pages back to the downgraded cache budget
- unknown load-profile ids fail allocation loudly

The shared `drs_phase1_pipeline_report` artifact now also includes a `loadProfile` section so CI shows whether profile selection, downgrade, and dormant-page purge still behave as expected for the reference corpus.
