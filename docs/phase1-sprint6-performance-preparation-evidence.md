# Mini Sprint 6.3 Completion Evidence

Date: July 19, 2026  
Decision: Pass

## Implemented evidence

- Added `PerformancePublishPreparation` as an all-or-nothing completion validator over immutable
  snapshot and worker results.
- Added deterministic route, source-provenance, and macro-schema digests alongside authored and
  prepared-content digests.
- Carried the new digests through typed results, controller accepted/active snapshots, activation
  payloads, prepared revisions, facade performance snapshots, and diagnostics.
- Rejected missing/duplicate/mismatched sources and zones, invalid handle/range bindings, partial
  topology, identity/build/revision drift, digest drift, and cancellation with path-scoped findings.
- Preserved source fingerprint/decode/cache work exclusively inside `PreparedPlaybackService`.
- Preserved last-known-good activation by converting every conformance failure into an explicitly
  ineligible contract result before staging.

## Focused validation

| Target | Result |
|---|---|
| `drs.sprint6.performance_preparation` | Passed |
| `drs.sprint6.publish_contract` | Passed |
| `drs.sprint6.publish_controller` | Passed |
| `drs.sprint6.publish_controller_integration` | Passed |
| `drs.sprint4_entry.authored_input` | Passed |
| Focused registered matrix | **Passed 5/5** |
| Direct expected-red audit | Expected exit 1 with exactly **6** later-sprint seams |
| Fully relinked Debug aggregate | **62/63 passed** in 183.03 seconds; sole callback-budget timing sample described below |
| Isolated strict realtime guard rerun | **Passed 1/1** in 32.94 seconds with unchanged thresholds |

The 6.3 matrix covers deterministic repeat results, stable-ID normalization independent of authored
collection order, two-source/two-zone WAV+FLAC topology, all-or-nothing partial rejection, route,
source-provenance, macro-schema, and revision invalidation, cancellation, structured paths, and
activation-payload lifetime. The inherited authored-input matrix performs real general-authored WAV
and FLAC fingerprint/decode/cache work in both Preview and Publish lanes and now proves cold/warm
digest equality plus full-project Publish conformance. The inherited realtime matrix remains the
authority for zero document/filesystem/decode work on the audio callback.

The fully relinked aggregate passed every functional, lifecycle, concurrency, diagnostics, state,
rendering, and new 6.3 target. Its only failure was one `drs.sprint4_entry.realtime_guard`
maximum-load callback-budget sample; allocation, free, lock, wait, file, path, decode, and large
release counters were all zero. The exact unchanged target then passed alone in 32.93 seconds. No
product rule or budget was weakened, and no test was skipped or waived.

## Exit assessment

Mini Sprint 6.3 exit criteria and Gate A3 are met. A successful result represents every authored
source and zone from one exact captured revision with deterministic immutable digests; partial,
selected-only, fixture-only, and mixed-revision payloads cannot become eligible. Mini Sprint 6.4 may
proceed with bounded cross-lane scheduling, cancellation, and budget enforcement.
