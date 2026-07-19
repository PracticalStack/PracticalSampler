# Sprint 4 Entry Gate Contract

Status: authoritative handoff contract, opened July 19, 2026.

This document is the Sprint 4 renderer team's compact source of truth for the boundary proven by
EG1 through EG5. The detailed preparation, activation, diagnostics, and real-time guard contracts
remain authoritative for their individual domains.

## Authored preparation boundary

- Preview and Publish submit immutable authored input to the preparation worker.
- The worker owns canonical-path resolution, actual source fingerprinting, WAV/FLAC decode,
  optional compiled-topology validation, cache lookup/fill, and structured findings.
- Reference-stream membership is not an eligibility condition. A deliberately container-free
  stream context is valid for decoded authored playback.
- Cache identity is compiler salt, canonical source identity, worker-observed fingerprint, and
  effective decode policy. Relink and same-path byte replacement therefore invalidate narrowly.
- Failed, canceled, stale, or superseded results are not activation eligible.

## Immutable activation boundary

- A successful Preview or Performance completion publishes one const activation payload containing
  revision/lifecycle identity, the immutable playback snapshot, and shared immutable prepared handles.
- Publication requires matching revision, build identity, snapshot digest, and prepared digest.
- The last-known-good payload survives rejected newer work and temporary worker-result destruction.
- The message owner fills a preallocated slot; the callback exchanges only integer slot indices.
- Old voices lease retired slots. Final shared release and large-resource destruction occur on the
  message-owned retirement path, never in the audio callback.

## Diagnostics boundary

- Callback-owned numeric facts use atomics and an odd/even sequenced frame.
- Document state, findings, strings, and snapshot composition are message-owned.
- Readers receive stable immutable snapshot values. They never inspect voice containers, retirement
  queue storage, mutable strings, or an aggregate being concurrently written.
- Preparation workers return normal facade completions; only message servicing translates them into
  processor diagnostics.

## Real-time enforcement boundary

- The callback is guarded against allocation, deallocation, lock entry, waiting, file I/O, path
  resolution, sample decode, stream decode, large-resource destruction, final shared release,
  capacity growth, and deadline overrun.
- Supported gate budgets are 44.1 or 48 kHz, block sizes 32 through 1024, up to 128 events per block,
  and 24 voices per Preview or Performance context.
- Each prohibited action has an isolated negative regression. The declared clean maximum-load case
  must report zero guard failures.

## Shell handoff

Standalone and editor-closed plug-in processor shells execute the same Preview/Publish scenario and
consume the same retained-payload contract. Their revision, build identity, digests, sample counts,
retained bytes, rendered-audio result, and guard totals must match.

## Sprint 4 assumptions

Sprint 4 may extract a shared renderer using const activation payload access and the existing bounded
slot exchange. It must preserve separate Preview and Performance mutable playback contexts, keep
payload retirement off audio, and retain every EG4 guard. The gate does not pre-implement the final
renderer, fixed voice-stealing policy, authored-performance cutover, or configurations outside the
declared budget profile.

## Detailed contracts

- [Authored preparation](phase1-sprint4-entry-authored-preparation-contract.md)
- [Immutable activation payload](phase1-sprint4-entry-activation-payload-contract.md)
- [Diagnostics ownership](phase1-sprint4-entry-diagnostics-ownership-contract.md)
- [Real-time guard harness](phase1-sprint4-entry-realtime-guard-contract.md)
- [Dated gate decision](phase1-sprint4-entry-gate-report-2026-07-19.md)
