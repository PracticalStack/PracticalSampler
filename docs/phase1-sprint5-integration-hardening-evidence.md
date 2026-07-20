# Mini Sprint 5.8 Completion Evidence

Date: July 19, 2026  
Decision: Pass

## Implemented evidence

- Removed duplicate authoring UI note-preview callbacks and shell fallback routing; both shells now
  submit the typed Preview command adapter exclusively.
- Removed processor-owned duplicate failed Preview revision/state fields. Compatibility diagnostics
  derive their message-side text from the controller's immutable snapshot.
- Added `AuthoringPreviewIntegrationBudgets` and enforced every supported bound in the registered
  `drs.sprint5.integration_hardening` target.
- Added mixed authored-edit/selection/audition churn with concurrent Performance rendering,
  immutable UI/diagnostics polling, deterministic worker reordering, activation retirement,
  close/reopen recovery, and exact Performance identity/output isolation.
- Added the closure target to `drs_all_tests` and to the Linux ThreadSanitizer workflow.

## Measured result

One representative Release run recorded:

| Measure | Result | Limit |
|---|---:|---:|
| Maximum request to audible | 975,259 microseconds | 8,000,000 microseconds |
| Retired / reclaimed payloads | 17 / 17 | Backlog no greater than 8, tail 0 |
| Performance / Preview peak voices | 1 / 3 | Both lanes exercised |
| Performance event/note drops | 0 / 0 | 0 |
| Preview event/note drops | 0 / 0 | 0 |
| Callback overruns | 0 | 0 |
| Realtime violations | 0 | 0 |

The deterministic controller reordering scenario separately proves that an older completion cannot
replace the newest request. The integrated physical worker remained within its fixed two-pending,
one-in-flight limits.

## Validation matrix

| Gate | Result |
|---|---|
| Fresh Debug configure and aggregate build | Passed; `drs_all_tests` built from `build/sprint5-closure-debug` |
| Fresh Debug full CTest | **Passed 59/59** in 207.83 seconds |
| Closure soak repetition | Passed twice in Debug and three consecutive times in Release after the final harness correction |
| Fresh Release VST3 build | Passed; `Decent Rhapsody Studio.vst3` produced |
| Release smoke + offline + closure + benchmark | **Passed 4/4** in 16.45 seconds |
| Sprint 4 inherited gates | Passed as part of the 59-test Debug matrix |
| ThreadSanitizer coverage | Closure target registered in `.github/workflows/thread-sanitizer.yml`; execution is delegated to Linux CI |

## Defects found during closure

Two test-harness issues were corrected without weakening production contracts:

1. The first soak loop could stop audio callbacks before the newest prepared result reached its
   block-boundary activation. It now continues until message-thread completion and the required
   render tail.
2. A tight 256-sample, non-realtime desktop loop could treat an OS scheduler preemption as DSP
   overrun. The integrated soak now uses the supported 1024-sample profile; the full realtime target
   retains the complete block-size matrix.

A cold-tree CTest attempt also produced transient timeouts, including one legacy waveform test that
immediately passed alone in 4.27 seconds. The final uninterrupted 59-test run passed, so no product
defect or waived test remains.

## Exit assessment

All Mini Sprint 5.8 tasks and gates P1-P8 are satisfied. Preview work is bounded, stale completions
cannot activate, failure preserves last-known-good, status reads are coherent, unsaved authored edits
do not change Performance, and the inherited Sprint 4 matrix remains green.
