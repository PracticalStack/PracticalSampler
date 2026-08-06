# Phase 2 WAV streaming vertical slice

Date: 2026-08-05

## Result

STR-200 through STR-203 are complete. Large over-budget WAV drafts enter a head-ready streaming path without fingerprinting or full PCM decode. The later actual-corpus qualification retained one Salamander zone/source in 1,586 us and prepared the full 641-source draft with zero decoded PCM bytes; see `../phase-7/accurate-salamander-qualification.md`.

## Implemented contract

- RIFF/RF64 parsing uses checked 64-bit byte and frame ranges and supports the current mono/stereo PCM16, PCM24, PCM32, and float32 policy.
- A sparse 5 GiB RF64 fixture validates a data offset of 80 bytes and preserves the full payload and frame count without allocating the payload.
- `WavPagedSampleDataSource` performs file reads and PCM-to-float conversion only from explicit worker preparation methods. The render-facing acquisition call performs bounded atomic lookups and never opens or reads a file.
- Heads are frame-aligned and capped at 16 KiB decoded float data; pages are capped at 64 KiB.
- The bounded scheduler deduplicates source-generation/page keys, upgrades priority, and deterministically displaces lower-priority work at capacity.
- Size and modification-time provenance is validated before reads. Changed sources fail without publishing stale head/page storage.
- Prepared playback chooses streaming when resident admission exceeds its byte budget, primes only the scoped dependency heads, and carries the immutable data-source descriptor into the common render model.

## Measured trace

Direct Debug worker-test output:

```text
Resident admission trace: salamanderScaleEstimateBytes=14768640000 residentBudgetBytes=536870912 fixtureEstimateBytes=705600 fingerprintOpens=0 fullFrameReads=0
WAV streaming trace: rangeReads=2 rangeBytes=40960 headResidentBytes=16384 pageResidentBytes=65536 scopedHeadBytes=16384 scopedSources=1 fullDecodedBytes=0
Slow preparation concurrency trace: maxStatusPollMicros=18 cancellationCommandMicros=48 shutdownMicros=386
Phase 1 prepared playback worker tests passed.
```

The range-read total covers one head read and one explicit page read in the source-level fixture. Production selected-zone preparation retained one 16 KiB head, retained no full decoded sample, issued no fingerprint open, and issued no full-frame read.

## Failure and recovery coverage

- Missing and four-byte truncated WAVs reject during descriptor construction.
- A WAV mutated after descriptor creation rejects during worker preparation and increments the source-mutation metric.
- Existing Sprint 5 recovery coverage verifies a failed new preparation does not replace the prior known-good activation.

## Verification commands

```powershell
cmake --build --preset build-debug --target drs_phase1_prepared_playback_worker_tests
build\vs2022-debug\tests\drs_phase1_prepared_playback_worker_tests_artefacts\Debug\drs_phase1_prepared_playback_worker_tests.exe
```

The build was run inside the repository's documented Visual Studio developer environment.
