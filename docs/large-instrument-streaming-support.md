# Large-Instrument Streaming Support

Status: implemented and release-qualified with the licensed Accurate Salamander corpus.

## Supported storage paths

| Input | Policy | Recovery |
|---|---|---|
| Authoring WAV | RIFF/RF64 PCM16/24/32 or float32, mono/stereo; 16 KiB decoded head and 64 KiB decoded pages | Relink a missing file. Reinspect/reprepare after size or modification-time change. |
| Package v2 (`DRSPKG2`) | 64-bit TOC offsets, independently authenticated metadata/head/page records, bounded range reads | Retry a transient read. Re-export when metadata or a required record is incompatible. A corrupt page is rejected locally. |
| Package v1 (`DRSPKG1`) | Resident compatibility only when the package is at most 64 MiB | Re-export larger v1 packages as v2. The reader never rewrites or searches nearby paths. |
| Small resident instruments | Common sample-data-source contract with a 512 MiB admission ceiling | Reduce the selected scope or use streaming when the estimate exceeds the ceiling. |

Defaults are policy inputs, not permanent format constants: resident admission is 512 MiB, WAV/package heads are 16 KiB, pages are 64 KiB, the page-cache default is 64 MiB, the callback intent ring holds 256 requests, and package v2 plaintext records are at most 64 KiB.

## Readiness and recovery

Package status is explicit: metadata loaded, opening sources, preparing heads, building model, playable, pending activation, active, degraded, failed, or cancelled. “Metadata loaded” never means audible. Plug-in and standalone use the same status formatter.

A failed replacement is degraded when an older package remains active; old voices retain their old model/source/page leases until their release tails finish. Missing source/page identity and retryability are included in the deferred-session diagnostic projection.

Host restore queues the persisted package locator. Locator resolution and file I/O belong to background service, not `setStateInformation()` or `processBlock()`.

## Operational diagnostics

Collect these fields for a support bundle:

- package format, package id/path, request and active generation;
- lifecycle stage, failure category, source id, page id, retryable flag;
- ready/total head counts;
- head/page hits and misses, queue depth/drops, maximum read latency;
- resident, leased, retired, and peak allocated page bytes;
- voice underrun/recovery counts;
- export processed/verification bytes, peak plaintext/sealed buffers, stage durations, throughput, and cancellation response;
- realtime allocation, deallocation, lock, wait, file/path/decode, ownership-release, and callback-budget counters.

## Qualification boundary

Synthetic and sparse tests cover 641 sources, 1,704 routes, approximately 1.84 GiB source audio, approximately 2.45 GiB decoded float residency, long samples, cache pressure, 5 GiB offsets, corruption, cancellation, activation replacement, and host locator deferral without storing large or licensed binaries.

Release qualification used the actual 641-WAV Accurate Salamander corpus (1,704 projected zones). The full draft prepared in 443,977 us with 10,502,144 resident-head bytes and zero full-corpus decoded bytes; peak process working set was 225,513,472 bytes. Normal selected-zone/group playback had zero misses and underrun frames, with a maximum 273 us callback against a 5,333 us budget. The 2,631,961,513-byte v2 export used 65,536/65,592-byte peak plaintext/sealed buffers and passed structural, open, audible-playback, cancellation, standalone, plug-in, and host-recall validation. The constrained-storage profile intentionally produced bounded silence and counters under a 75 ms service poll, without waits, hangs, or crashes.

The environment supported a first post-export open and a warm metadata reopen, but not a privileged operating-system filesystem-cache flush; the report therefore does not claim a controlled cold-cache measurement. See `artifacts/large-instrument-streaming/phase-7/accurate-salamander-qualification.md` and the adjacent Phase 7 host/corruption/soak evidence.
