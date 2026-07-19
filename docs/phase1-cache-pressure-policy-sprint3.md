# Phase 1 Prepared Cache Pressure Policy Note

This note captures Sprint 3 task `S3.6-T2` from section 6.1 of `engineering-plan.html`: define a reviewable cache pressure policy for prepared playback content.

## Policy

- retention target:
  - keep enough prepared ownership bytes for two working sets
  - rationale: one active ready set plus one replacement set during preview/publish churn
- working-set baseline:
  - the largest of preview prepared ownership bytes, published prepared ownership bytes, and current active prepared-cache ownership bytes
- byte budget:
  - `workingSetBytes * 2`
- resident bytes:
  - active prepared ownership bytes plus retired prepared ownership bytes awaiting cleanup

## Shell-facing state

- `Nominal`
  - resident bytes are at or below the current working-set baseline
- `Replacement set retained`
  - resident bytes exceed one working set but remain within the two-working-set budget
- `Over budget`
  - resident bytes exceed the two-working-set budget
- `Idle`
  - no prepared ownership bytes are currently resident

## Product effect

- the worker now reports active prepared ownership bytes in addition to record counts and retired bytes
- diagnostics, performance snapshots, and shell detail expose:
  - retention working-set count
  - working-set bytes
  - byte budget
  - resident bytes
  - headroom bytes
  - pressure state

## Regression coverage

- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - verifies active ownership bytes stay stable across retirement cleanup
- `tests/src/Phase1DiagnosticsTests.cpp`
  - verifies default and restored diagnostics expose the two-working-set policy and remain within budget
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - verifies diagnostics/performance alignment and shell detail reporting for the policy
- `tests/src/Phase0SmokeTests.cpp`
  - verifies the shell detail includes the prepared cache policy line
