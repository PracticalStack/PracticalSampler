# STR-002 Resident Admission and Honest Readiness Evidence

Date: 2026-08-05  
Status: verified complete

## Typed admission contract

`ResidentPreparationAdmissionResult` exposes:

- typed readiness: `metadataLoaded`, `playbackDeferred`, `playable`, or `streamingRequired`;
- checked estimated decoded bytes;
- configured resident budget bytes;
- metadata/admission/overflow flags;
- stable finding code and actionable guidance.

The estimator uses checked 64-bit `frameCount × channelCount × sizeof(float)` multiplication plus checked aggregation. `PreparedPlaybackService` performs metadata-only inspection across the whole retained scope before any source fingerprint or PCM read. Requests above `PreparedPlaybackSchedulerBudgets::maximumRetainedPreparedBytes` (default 512 MiB) are rejected as `streamingRequired`.

`DraftPlaybackContract` retains the typed readiness and admission numbers. Package workspace state separately exposes typed readiness and a `playable` flag. Manifest-only workspaces are `metadataLoaded` and not playable; successful immutable package activation is `playable`. Plug-in and standalone status text maps those states without claiming that metadata is playable.

## Over-budget trace

Direct Debug trace from `drs_phase1_prepared_playback_worker_tests`:

```text
salamanderScaleEstimateBytes=14768640000
residentBudgetBytes=536870912
fixtureEstimateBytes=705600
fingerprintOpens=0
fullFrameReads=0
```

The Salamander-scale synthetic metadata set contains 641 one-minute, 48 kHz, stereo sources. It deterministically returns `resident-admission-budget-exceeded`. A separate maximum-`uint64_t` fixture returns `resident-admission-size-overflow`.

The production-path fixture applies a one-byte resident budget to the checked-in authored project. It rejects before fingerprinting or full-frame reading, proving admission precedes PCM allocation.

## Readiness verification

- `drs.phase0.smoke` asserts manifest-only plug-in and standalone workspaces are metadata-loaded and not playable.
- `drs.phase1.performance_package_session` asserts both shells become playable only after a real immutable package activation succeeds.
- Shell status text distinguishes package metadata, playback deferred, streaming required, and playable.

## Regression commands and results

The targeted build succeeded. A 15-test admission/smoke/resident/Sprint 5/package run passed 14 tests; `drs.sprint5.preview_shell_parity` had one Debug segfault in the aggregate run, then passed three consecutive isolated repetitions. This matches the baseline's broader Debug lifecycle instability and is recorded rather than hidden.

Focused tests all pass:

- `drs.phase0.smoke`
- `drs.phase1.prepared_playback`
- `drs.phase1.prepared_playback_worker`
- `drs.phase1.performance_package_session`
- `drs.sprint4.offline_renderer`
- all ten `drs.sprint5.*` tests, with shell parity repeated three times in isolation

## Files

- `engine_adapter/include/drs/engine/PreparedPlayback.h`
- `engine_adapter/src/PreparedPlayback.cpp`
- `engine_adapter/include/drs/engine/SampleImport.h`
- `engine_adapter/src/SampleImport.cpp`
- `engine_adapter/include/drs/engine/DraftPlaybackContract.h`
- `engine_adapter/src/DraftPlaybackContract.cpp`
- `engine_adapter/include/drs/engine/PerformancePackage.h`
- `app/src/plugin/PluginProcessor.cpp`
- `app/src/plugin/PluginEditor.cpp`
- `app/src/standalone/MainComponent.cpp`
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
- `tests/src/Phase1PreparedPlaybackTests.cpp`
- `tests/src/Phase0SmokeTests.cpp`
- `tests/src/Phase1PerformancePackageSessionTests.cpp`
