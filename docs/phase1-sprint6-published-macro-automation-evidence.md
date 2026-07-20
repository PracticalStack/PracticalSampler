# Mini Sprint 6.7 Completion Evidence

Date: July 20, 2026  
Decision: Pass

## Implemented evidence

- Added an immutable published macro table keyed by stable authored ID and fixed host slot.
- Bound table revision/schema identity into controller authorization and activation eligibility.
- Added deterministic compatible-value preservation, range clamping, authored defaults for new
  bindings, retirement, reordering, renaming, and explicit unassigned handling.
- Exchanged the fixed callback view with the sampler model at the activation block boundary.
- Routed active host automation through fixed atomic slot values into per-block note-on controls,
  without render-model rebuild or host topology mutation.
- Preserved Draft/Preview-only authored schema changes until explicit Publish.
- Reduced the Sprint 6 expected-red audit from five to four later-sprint seams.

## Dedicated matrix

`drs.sprint6.published_macro_binding` proves:

- initial preset/current values bind by stable ID;
- reordered and renamed compatible controls preserve identity;
- narrowed ranges clamp deterministically;
- removed controls retire and added incompatible controls remain explicitly unassigned;
- a re-added binding uses its authored default instead of a retired value;
- duplicate IDs, invalid ranges, and over-capacity schemas reject publication;
- the declared maximum macro count stays bounded;
- authored schema changes do not alter active bindings or host parameter count before Publish;
- audio revision and callback-view revision change together at one boundary; and
- automation immediately before the boundary remains old-schema while automation immediately
  after the boundary addresses the new published binding.

## Validation results

| Check | Result |
| --- | --- |
| Debug `drs_all_tests` aggregate build | Passed; 118 affected compile/link steps completed with the 6.7 target included. |
| CTest discovery | Passed; 67 tests discovered and 6.7 is registered as test 38. |
| Dedicated 6.7 matrix | Passed directly and through CTest in 3.05 seconds. |
| Sprint 6 plus strict realtime gates | Passed 10/10 in 63.04 seconds, including realtime safety and the 33-second entry guard. |
| Sprint 4 renderer/voice/context cluster | Passed 7/7 in 10.19 seconds. |
| Concurrency soak and Sprint 5 integration hardening | Passed 2/2 in 20.79 seconds. |
| Expected-red seam audit | Expected exit 1 with exactly four Mini Sprint 6.8 seams. |

The first mixed CTest batch reported transient process crashes in the shell-cutover and generation
executables. Both passed immediately when run directly and then passed in the authoritative
isolated CTest clusters above; no deterministic failure remained.

## Exit assessment

Mini Sprint 6.7 exit criteria are met: audio and automation use one published macro schema/revision
at a time, migration is stable-ID based, unassigned/retired controls cannot affect Performance,
and the host parameter topology remains fixed. Mini Sprint 6.8 may proceed with typed Publish
commands, presentation/status truth, routing audits, and shell parity.
