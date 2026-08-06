# Phase 5 streaming export

Date: 2026-08-05

## Result

STR-500 through STR-503 are complete. The later actual-corpus run exported and structurally verified a 2,631,961,513-byte Salamander v2 package with 40,865 records and 65,536/65,592-byte peak plaintext/sealed buffers; see `../phase-7/accurate-salamander-qualification.md`.

## Pipeline

- The export-plan builder maps already-compiled float payload ranges to a 16 KiB head and 64 KiB pages without loading the payload.
- Every record has a lazy exact-range loader; package writing materializes one bounded plaintext record, seals it, writes it, and releases it before the next record.
- Only fixed-size header state and bounded record metadata scale with page count. No audio-size vector exists.
- The writer creates `<output>.stage`, writes a 64 KiB-chunked TOC placeholder with cancellation checks, appends records, patches header/TOC, flushes, and closes.
- Verification reopens the stage, validates all ranges/identities, and authenticates/checksums the first and last records. It never rereads the full package.
- Windows publication uses replace-existing plus write-through semantics after verification. Cancellation and failure remove the stage; the requested output is never made publishable early.

## Metrics

```text
Streaming export trace: records=6 plaintextBytes=184 peakPlaintextBytes=64 peakSealedBytes=120 verificationBytes=1040 totalMicros=3393 throughputBps=54229.3 sparseCancelMillis=2332
```

The tiny deterministic fixture deliberately uses small records; policy caps remain 65,536 plaintext bytes and a correspondingly bounded sealed record. The result reports load, seal, write, verification, total durations, plaintext throughput, completed records, processed bytes, peak plaintext/sealed buffers, package bytes, and verification bytes.

## Large synthetic cancellation

A sparse 1 GiB compiled-float input produces a 64-bit streaming plan with 16 KiB head and 64 KiB pages. Cancellation during chunked TOC staging completed in 2,332 ms on the Debug Windows filesystem path, before allocating any audio plaintext/sealed record buffer. The stage and target output were absent afterward. Cancellation is worker-owned; no message/audio callback performs export I/O.

## Verification

```powershell
cmake --build --preset build-debug --target drs_package_v2_tests
build\vs2022-debug\tests\drs_package_v2_tests.exe
```

Result: `Package v2 bounded record matrix passed.`
