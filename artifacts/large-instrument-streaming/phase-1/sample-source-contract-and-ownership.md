# Phase 1 Sample Source Contract and Ownership

Date: 2026-08-05  
Packages: STR-100 through STR-103  
Status: verified complete

## Contract

`SampleDataSourceDescriptor` v1 carries stable source/canonical/provenance identity, source kind, format/layout, checksum, generation, sample rate, 64-bit frame/data ranges, bytes-per-frame mapping, and configurable head/page sizes (16 KiB/64 KiB defaults).

Validation rejects unsupported versions, missing identity, zero generation/dimensions, unsupported channel count, invalid rate, zero/overflowing frame mapping, overflowing offset ranges, truncated declared data, and zero head/page sizes.

`ISampleDataSource::acquireFrameView()` is `noexcept` and returns a bounded non-owning mono/stereo view with explicit `ready`, `endOfSource`, `pageMissing`, or `failed` status. Implementations:

- `ResidentSampleDataSource` adapts existing immutable decoded data.
- `DeterministicFakePagedSampleDataSource` exposes fixed heads/pages and deterministic missing-page behavior without I/O.

The immutable render model retains both the common source and the bounded decoded-resident compatibility owner. `SamplerVoice` reads only through the source contract. Existing route, DSP, loop, pitch, release, pan, crossfade, RR, choke, and activation behavior therefore share the same voice path.

## Ownership invariants

1. Prepared activation owns immutable resident data and source provenance.
2. Render model owns the source adapter and retains the activation generation.
3. Playback context owns the active render model across callback cutover.
4. Voice holds only model/route/sample pointers while its model generation remains retained by the context.
5. Each ready head/page view obtains a lock-free atomic `SamplePageLease`; eviction/retirement must observe a zero pin count before reclaiming page storage.
6. Lease copies increment the pin; destruction decrements it. No lease acquisition allocates, locks, performs I/O, or throws.
7. Retired generation destruction remains an off-audio responsibility of the playback context/cache retirement queues.

## Evidence

`drs.sprint4.render_model` proves:

- a descriptor representing 3,000,000,000 stereo frames validates without allocating PCM;
- checked 64-bit overflow is rejected;
- resident views expose exact bounded frames;
- fake paged views distinguish head, ready page, missing page, and end;
- page lease counts remain pinned across copied views and return to zero afterward;
- render-model descriptors validate and retain the common data source.

`drs.sprint4.voice_kernel` proves the voice start/render methods remain `noexcept` and preserve the resident formulas through the new view path.

`drs.sprint4.offline_renderer` passes the existing golden baseline bit-identically.

Focused result: 3/3 passed in 0.30 seconds.

## Files

- `engine_adapter/include/drs/engine/SampleDataSource.h`
- `engine_adapter/src/SampleDataSource.cpp`
- `engine_adapter/include/drs/engine/SamplerRenderModel.h`
- `engine_adapter/src/SamplerRenderModel.cpp`
- `engine_adapter/src/SamplerVoice.cpp`
- `engine_adapter/CMakeLists.txt`
- `tests/src/Sprint4RenderModelTests.cpp`
