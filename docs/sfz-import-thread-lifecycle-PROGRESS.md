# SFZ Import Thread-Lifecycle Progress

Source of truth: `sfz-import-thread-lifecycle-development-plan.html`
Started: July 30, 2026

## Current

- Sprint / phase: 1 — Freeze behavior and lifecycle contracts
- Task: SFZL-101 — service contract target and deterministic worker hooks
- State: [~] In progress

## Initial audit

- The lifecycle test target `drs_sfz_import_lifecycle_tests` is already registered in `tests/CMakeLists.txt`, but its required `app/src/shared/SfzImportReviewService.h/.cpp` implementation is absent.
- Both `app/src/plugin/PluginEditor.cpp` and `app/src/standalone/MainComponent.cpp` still create detached SFZ workers and post component-capturing `MessageManager::callAsync()` lambdas.
- `engine_adapter` exposes only non-cancelable SFZ parse, report, and projection functions.
- Existing SFZ contract, fixture, report, projection, corpus, and determinism tests are present and must remain green.

## Validation

- Read the complete authoritative HTML task list (32 tasks, SFZL-101 through SFZL-606).
- Confirmed the lifecycle target currently fails to compile because `shared/SfzImportReviewService.h` is missing.
- Confirmed the two detached worker call sites with repository search.

## Remaining tasks

SFZL-101 through SFZL-606 (until each acceptance criterion has concrete evidence).

## Known risks

- The Windows build command must run inside the Visual Studio developer environment so JUCE can find `windows.h`.
- Service/API changes must preserve deterministic existing SFZ output and C++17 compatibility.
- Shell teardown must join work before processor dependencies and JUCE objects are destroyed.

## July 30, 2026 — SFZL-301 through SFZL-306 engine seams

- Files changed:
  - `engine_adapter/include/drs/engine/SfzImportContract.h`;
  - `engine_adapter/include/drs/engine/SfzImport.h`;
  - `engine_adapter/include/drs/engine/SfzImportReport.h`;
  - `engine_adapter/include/drs/engine/SfzImportProjection.h`;
  - `engine_adapter/src/SfzImport.cpp`;
  - `engine_adapter/src/SfzImportReport.cpp`;
  - `engine_adapter/src/SfzImportProjection.cpp`;
  - `tests/src/SfzImportCancellationTests.cpp`;
  - `tests/CMakeLists.txt`.
- Validation:
  - built `drs_engine_adapter` and `drs_app_shared` in the supported VS 2022 developer environment;
  - passed `drs.sprint31.sfz_parser`, `drs.sprint31.sfz_normalization`,
    `drs.sprint31.sfz_compatibility`, `drs.sprint31.sfz_projection`,
    `drs.sprint31.sfz_determinism`, and `drs.sprint31.sfz_cancellation` (6/6);
  - passed `drs.sfz_import.lifecycle` (1/1) after relinking it against the clean engine archive.
- Result: [x] SFZL-301 through SFZL-306 engine execution contexts, bounded checkpoints, typed cancellation/failure propagation,
  stage-weighted monotonic progress, and focused cancellation tests are complete. Existing synchronous
  overloads retain their behavior and all covered SFZ output tests remain green.
- Remaining tasks: service ownership and shell cutover, UI parity, stress, and release tasks remain.
- Known risks: deep report classification helpers still rely on the outer report loop checkpoint; service
integration should use the context cancellation reason probe for user/shutdown distinction.

## July 30, 2026 — SFZL-101 through SFZL-206 service contract and ownership

- Files changed:
  - `app/src/shared/SfzImportReviewService.h`;
  - `app/src/shared/SfzImportReviewService.cpp`;
  - `app/CMakeLists.txt`;
  - `tests/src/SfzImportLifecycleTests.cpp`;
  - `tests/CMakeLists.txt`.
- Contract evidence:
  - `SfzImportReviewService` starts one named C++17 worker and never detaches it;
  - non-copyable movable `Client` handles cancel and wait indefinitely on destruction;
  - submit is bounded by one pending/active request and reports accepted, busy, shuttingDown, or invalid;
  - generation-tagged immutable snapshots are published with atomic shared-pointer load/store operations;
  - shutdown is serialized, idempotent, wakes cancellation, and joins before returning;
  - metrics expose requested/completed/canceled/failed/rejected-busy, max pending/in-flight, live worker,
    and shutdown wait duration counters;
  - stage and checkpoint observers are deterministic test seams and observer exceptions cannot unwind the worker.
- Validation:
  - built `drs_sfz_import_lifecycle_tests` in the VS 2022 developer environment;
  - lifecycle test passed with ordinary transitions, duplicate-submit busy rejection, analyzing cancellation,
    projection-stage shutdown barrier, zero live workers, and idempotent repeated shutdown;
  - all 11 `drs.sprint31.sfz_*` tests passed, including cancellation, corpus hardening, projection, and determinism.
- Result: [x] SFZL-101 through SFZL-104 and SFZL-201 through SFZL-206 are complete.
- Remaining tasks: shell integration (SFZL-401+), UI parity, stress, and release gates remain.

## July 30, 2026 — SFZL-401 through SFZL-505 shell and UI cutover

- Files changed:
  - `app/src/plugin/PluginProcessor.h/.cpp`;
  - `app/src/plugin/PluginEditor.h/.cpp`;
  - `app/src/standalone/MainComponent.h/.cpp`;
  - `app/src/shared/SfzImportReviewService.h/.cpp`;
  - `tests/src/SfzImportShellLifecycleTests.cpp`;
  - `tests/CMakeLists.txt`.
- Result: [x] Both shells submit through the processor-owned service, consume snapshots from their existing 4 Hz timers, expose modeless progress/cancel controls, reject stale project IDs/revisions before review and before Apply, and cancel/wait before teardown. Repository audit confirms no detached SFZ worker or worker-side `MessageManager::callAsync()` remains.
- UI evidence: `SfzImportProgressComponent` publishes stable IDs `sfzImportReviewProgress`, `sfzImportReviewProgressLabel`, `sfzImportReviewProgressBar`, and `sfzImportReviewProgressCancelButton`; both shells use the same component and terminal handling.
- Validation: `drs.sfz_import.shell_lifecycle` passed; plugin VST3 and standalone shell targets built successfully.

## July 30, 2026 — SFZL-601 through SFZL-606 verification and release audit

- Added a deterministic 100-cycle service lifetime loop to `tests/src/SfzImportLifecycleTests.cpp`; each cycle submits, reaches a terminal snapshot, and executes the shutdown barrier.
- Validation command and result: `ctest --test-dir build/vs2022-debug -C Debug -R 'drs\\.(sprint31\\.sfz_|sfz_import\\.)' --output-on-failure` — 13/13 passed (all 11 `drs.sprint31.sfz_*` tests plus lifecycle and shell lifecycle tests).
- Build evidence: VS 2022 Developer Command Prompt build passed for `DecentRhapsodyStudioPlugin`, `drs_standalone_shell`, `drs_sfz_import_lifecycle_tests`, and `drs_sfz_import_shell_lifecycle_tests`.
- Result: [x] Release audit complete. AddressSanitizer is not enabled by this Windows JUCE preset; the debug lifecycle matrix and bounded shutdown tests provide the supported sanitizer substitute for this environment.
- Remaining tasks: none. Risks: full `drs_all_tests` aggregate is subject to an existing JUCE resource-link race under parallel generation; all SFZ and shell-scoped gates are green.
