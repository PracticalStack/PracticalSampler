# STR-001 Lock Scope and Cancellation Evidence

Date: 2026-08-05  
Status: verified complete

## Lock audit and implementation

`PreparedPlaybackService::prepare()` no longer holds `workerMutex` across source fingerprinting, decode/conversion, digest construction, or cooperative cancellation checks.

The mutex is now limited to bounded coordination sections:

- queue dispatch and completed-result publication;
- cache lookup snapshots and cache-entry publication/retirement;
- status counter publication;
- cancellation cleanup of entries owned by the canceled build.

Fingerprinting receives the lane cancellation generation through `SampleFingerprintCallbacks`, so a queued cancel or service destructor interrupts chunked fingerprint work. Destruction advances both lane generations before requesting worker stop and joining.

## Deterministic slow-preparation trace

`drs.phase1.prepared_playback_worker` creates a sparse 1 GiB source and starts a background Preview fingerprint. While it is deliberately in flight, the test polls worker status 64 times, cancels a second run, and destroys a third worker.

Measured Debug trace:

```text
maxStatusPollMicros=20
cancellationCommandMicros=18
shutdownMicros=310
```

Budgets asserted by the test:

- every status poll: less than 16,000 µs;
- cancellation command: less than 16,000 µs;
- shutdown while in flight: less than 250,000 µs;
- canceled work publishes exactly one canceled completion and does not fingerprint the whole sparse fixture.

Build/run command:

```text
cmake --build --preset build-debug --target drs_phase1_prepared_playback_worker_tests
build\vs2022-debug\tests\drs_phase1_prepared_playback_worker_tests_artefacts\Debug\drs_phase1_prepared_playback_worker_tests.exe
```

Result: passed in 0.37 seconds under CTest; direct trace run passed.

## Files

- `engine_adapter/src/PreparedPlayback.cpp`
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
