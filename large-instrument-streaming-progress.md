# Large-Instrument Streaming Progress Ledger

Authoritative plan: `large-instrument-streaming-iteration-plan.html`  
Started: 2026-08-05  
States: `[ ]` not started, `[~]` in progress, `[x]` verified complete, `[!]` blocked.

## Resume summary

- Current critical path: complete; final source/plan/test audits passed.
- All Phase 0 through Phase 7 work packages and corpus-dependent gates are verified complete.
- The plan, repository README files, build guide, CMake presets, and complete STR/gate/decision inventory were read before implementation.
- User-owned pre-existing files: the untracked HTML plan and `validation/large-instrument-plan-*.png`; preserve them.
- Licensed Accurate Salamander corpus was qualified from the external `DemoSFVInstruments` tree. The derived 2.63 GB package was deleted after host qualification; no licensed data is retained in the repository.
- Baseline evidence: `artifacts/large-instrument-streaming/phase-0/baseline-assessment.md`.

## Initial repository assessment

| Area | Production seam |
|---|---|
| Preparation/snapshots | `PlaybackSnapshot`, `PreparedPlaybackService`, `AuthoringPreviewPreparation`, `DraftPlaybackContract` |
| Render models/voices | `SamplerRenderModel`, `SamplerVoice`, `SamplerVoicePool`, `SamplerPlaybackContext` |
| Audio callback | `app/src/plugin/PluginProcessor.cpp::processBlock`, sampler playback contexts, `app/src/plugin/RealtimeGuard.h` |
| WAV inspection/decode | `SampleImport`, `WavImportService`, `WaveformPreviewService` |
| Package I/O | `PackageReader`, `PackageWriter`, `PerformancePackageExportService` |
| Crypto/authentication | `PackageCrypto` and package reader/writer failure taxonomy |
| Activation generations | `DraftPlaybackContract`, `PerformanceLaneState`, `SamplerPlaybackContext`, `ProjectRestoreCoordinator` |
| Plug-in/standalone lifecycle | `PluginProcessor`, `PluginEditor`, `MainComponent` |
| Tests/fixtures | `tests/CMakeLists.txt`, `tests/src`, `tests/support`, `content/runtime` |

### Baseline commands and results

- `cmake --preset vs2022-debug` from a plain PowerShell: failed before configuration because the shell lacked SDK library paths (`LNK1181 kernel32.lib`). This is avoided by the documented bootstrap.
- `powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -RunTests`: configured and built all Debug targets. CTest finished with 65/172 failures after the exact `drs_host_state_restore_stress_tests.exe` process was bounded at 912.69 seconds; failures pre-date streaming changes.
- Isolated confirmation: `ctest --test-dir build\vs2022-debug --output-on-failure -R "^(drs.phase0.smoke|drs.phase1.prepared_playback|drs.sprint4.offline_renderer)$"`: smoke failed, prepared playback failed, resident offline renderer passed.
- `powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -Configuration Release -RunTests`: configured and built; 118/172 passed, 54 failed/not-run. Several targets are absent from `drs_all_tests`; several lifecycle binaries terminate with stack/segfault status. Core smoke/preparation defects reproduce in Release.
- Resident baseline: `drs.sprint4.offline_renderer` passes against `tests/baselines/sprint4-offline-render-baselines.txt`.

## Phase 0 — Stabilize and Bound

Outcome: selected scopes are bounded before realization, coordination locks remain responsive, and resident-budget admission/readiness is honest.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-000 | Scope-aware snapshot/preparation request plus selected-zone/group regressions | Scope/dependency closure now runs before prepared realization; current-draft remains explicit and deterministic. | `PlaybackSnapshot.*`, `DraftPlaybackContract.*`, `EngineFacade.*`, `PluginProcessor.cpp`, Sprint 5 preparation/controller/coalescing tests, Phase 0 smoke | Targeted Debug build succeeded; 12/12 smoke/resident/Sprint 5 tests passed. Selected zone 3→1 source, group 3→2 zones/1 source, full draft 3 zones/2 sources; synthetic dependency closure 5→4. | `artifacts/large-instrument-streaming/phase-0/str-000-scoped-preparation.md` |
| [x] | STR-001 | Lock-scope audit, concurrency tests, message-thread latency trace | Long fingerprint/decode/digest work runs outside `workerMutex`; cache/status publish in bounded sections; fingerprint and destructor use cooperative lane cancellation. | `PreparedPlayback.cpp`, `Phase1PreparedPlaybackWorkerTests.cpp` | 1 GiB sparse slow-preparation probe: max status poll 20 µs, cancellation command 18 µs, shutdown 310 µs; worker test passed. | `artifacts/large-instrument-streaming/phase-0/str-001-lock-scope-and-cancellation.md` |
| [x] | STR-002 | Typed admission result, UI mapping, over-budget fixtures | Checked metadata-only admission precedes fingerprint/PCM work; 512 MiB is enforced; draft/package/shell readiness is typed and honest. | `PreparedPlayback.*`, `SampleImport.*`, `DraftPlaybackContract.*`, `PerformancePackage.h`, processor/editor/standalone status, admission/package/smoke tests | 641-source synthetic estimate 14,768,640,000 bytes rejected vs 536,870,912 budget; one-byte production fixture had 0 fingerprint opens and 0 full-frame reads; overflow test passes; metadata-only shell not playable. | `artifacts/large-instrument-streaming/phase-0/str-002-resident-admission.md` |

### Phase 0 acceptance gate

- [x] Selected-zone preview decodes only the retained dependency set.
- [x] Status polling remains below 16 ms during deliberately slow preparation.
- [x] Cancellation and shutdown complete without waiting for the full corpus.
- [x] The Salamander full-draft request selects streaming before PCM allocation: 2,618,648,472 estimated resident bytes, zero full-draft decoded bytes, and 10,502,144 resident-head bytes.
- [x] Metadata-only packages are never labeled playable.

### Phase 0 evidence

- [x] V0.1 Scoped decode/read counters (`str-000-scoped-preparation.md`).
- [x] V0.2 Message-thread latency trace (`str-001-lock-scope-and-cancellation.md`).
- [x] V0.3 Over-budget admission test (`str-002-resident-admission.md`).
- [x] V0.4 Cancel/close concurrency soak (`str-001-lock-scope-and-cancellation.md`).

## Phase 1 — Sample Data Source Contract

Outcome: render models use stable descriptors/bounded views; small resident instruments use the same contract.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-100 | Versioned `SampleDataSourceDescriptor` and validation | v1 descriptor carries stable identity, source kind, format/layout/checksum/generation, 64-bit ranges, and 16 KiB/64 KiB defaults with checked validation. | `SampleDataSource.h/.cpp`, render-model tests | Multi-gig descriptor valid without PCM; overflow/range failures covered. | `artifacts/large-instrument-streaming/phase-1/sample-source-contract-and-ownership.md` |
| [x] | STR-101 | Source interface, resident adapter, deterministic fake paged adapter | `noexcept` bounded views return ready/end/missing/failed; resident and deterministic paged adapters share the interface. | `SampleDataSource.*`, render-model tests | Head/page/end/failure and resident exact-view matrix passes. | Same Phase 1 artifact. |
| [x] | STR-102 | Dual-mode prepared/render model with unchanged routes | Render samples retain common source descriptors/handles plus bounded resident compatibility ownership; voices read only source views. | `SamplerRenderModel.*`, `SamplerVoice.cpp` | Voice kernel and resident golden offline renderer pass unchanged. | Same Phase 1 artifact. |
| [x] | STR-103 | Lifecycle state machine and ownership invariants | Activation→model→voice ownership specified; lock-free page lease pins prevent reclamation while a view is active; retirement remains off audio. | `SampleDataSource.*`, ownership artifact/tests | Atomic pin is always lock-free; copied lease counts 1→2→0 deterministically. | Same Phase 1 artifact. |

### Phase 1 acceptance gate

- [x] Resident fixtures render bit-identically through the new contract.
- [x] A synthetic multi-gigabyte source can be described without allocating its PCM size.
- [x] Audio-facing reads are lock-free, allocation-free, and exception-free.
- [x] Generation retirement cannot invalidate an active page lease.

Required artifacts/evidence: source contract, resident adapter, ownership specification, contract and realtime-guard tests.

## Phase 2 — Real Authoring WAV Streaming

Outcome: large SFZ drafts become preview-capable from bounded WAV heads/pages without corpus decode.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-200 | Seekable WAV descriptor builder with RF64-safe offsets | Pure RIFF/RF64 chunk parser maps PCM16/24/32 and float32 mono/stereo data with checked 64-bit offsets/frame counts and source provenance. | `SampleDataSource.h/.cpp`, `Phase1PreparedPlaybackWorkerTests.cpp` | Real WAVs plus a sparse 5 GiB RF64 fixture pass exact offset/size/frame assertions; missing/truncated inputs reject. | `artifacts/large-instrument-streaming/phase-2/wav-streaming-vertical-slice.md` |
| [x] | STR-201 | Real WAV page provider and scheduling tests | Worker-only range read/conversion; callback performs atomic view lookup only. Bounded scheduler deduplicates and prioritizes head/imminent/lookahead work. | `SampleDataSource.h/.cpp`, `Phase1PreparedPlaybackWorkerTests.cpp` | Head/page boundary conversion matches resident decode; capacity-2 scheduling and priority displacement pass. | Same Phase 2 evidence artifact. |
| [x] | STR-202 | Scoped head-preparation pipeline and startup metrics | Over-budget WAV preparations select streaming, prime only scoped 16 KiB heads, and publish playable activation without full decode/fingerprint. | `PreparedPlayback.h/.cpp`, `SamplerRenderModel.cpp`, worker tests | Trace: scoped sources=1, head=16,384 bytes, decoded=0, fingerprint opens=0, full-frame reads=0. | Same Phase 2 evidence artifact. |
| [x] | STR-203 | Source-generation invalidation/recovery coverage | Descriptor binds size/mtime generation; worker validates provenance before head/page reads and never publishes mismatched data. Existing recovery keeps last known-good activation. | `SampleDataSource.cpp`, `Phase1PreparedPlaybackWorkerTests.cpp`, Sprint 5 recovery coverage | Post-descriptor mutation fails with mutation counter; missing/truncated inputs fail; recovery suite covers retained prior activation. | Same Phase 2 evidence artifact. |

### Phase 2 acceptance gate

- [x] Selected Salamander zones reach head-ready without full-corpus decoding (1 zone/source in 1,586 us; full draft decoded zero bytes).
- [x] Measured head residency stays within the scoped budget.
- [x] No source read or conversion occurs on the audio thread.
- [x] Missing, changed, and truncated WAVs fail without invalidating the previous good activation.

### Phase 2 evidence

- [x] V2.1 Real WAV range-read trace.
- [x] V2.2 Head residency accounting.
- [x] V2.3 Slow-disk and mutation tests.

## Phase 3 — Paged Sampler Rendering

Outcome: voices start in heads, cross ready pages, publish bounded look-ahead intents, and degrade without callback waits.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-300 | Paged voice kernel with resident parity tests | Production voice reads bounded source views for current/next interpolation across head/page boundaries. | `SamplerVoice.h/.cpp`, `Sprint4VoiceKernelTests.cpp` | Resident vs all-ready paged render matches at 1e-6; existing offline golden passes. | `artifacts/large-instrument-streaming/phase-3/paged-rendering-and-cache.md` |
| [x] | STR-301 | Bounded request ring and scheduler integration | Fixed 256-entry SPSC ring; proactive cadence and miss intents; worker dedupe, priority, and generation cancellation. | `SampleDataSource.h/.cpp`, voice and worker tests | Exact-capacity FIFO/drop test and voice look-ahead publication pass; obsolete-generation cancellation passes. | Same Phase 3 artifact. |
| [x] | STR-302 | Deterministic underrun policy and fault suite | Missing frames produce bounded silence while position/release advance; ready data resumes at current time with voice/block counters. | `SamplerVoice.h/.cpp`, `SamplerVoicePool.cpp`, voice tests | Three injected misses advance to frame 6; frames 6–7 resume with one recovery, no replay/hang. | Same Phase 3 artifact. |
| [x] | STR-303 | Production page cache with pressure/retirement metrics | Page-byte budget, pinned heads/leases, off-audio LRU, safe acquisition/reclamation handshake, and resident/leased/retired diagnostics. | `SampleDataSource.h/.cpp`, worker tests | 64 KiB budget peaks at exactly 64 KiB; pinned replacement rejects; release permits one eviction; no overshoot. | Same Phase 3 artifact. |
| [x] | STR-304 | Streamed performance-engine conformance matrix | Added ready-paged layered polyphony/release/allocation matrix and ran established voice/offline/crossfade/performance suites. | `Sprint4VoicePoolTests.cpp` plus existing matrices | Paged two-layer pool passes with zero callback allocation/deallocation. The final keyswitch, pedal, round-robin, voice, offline, crossfade, scheduler, and realtime matrices pass in Debug/Release. | Same Phase 3 artifact plus `phase-7/realtime-lifecycle-soak.md`. |

### Phase 3 acceptance gate

- [x] Resident and streamed renders match within the defined numeric tolerance.
- [x] No callback allocation, lock, file operation, decryption, or wait is observed.
- [x] Cache residency never exceeds the configured byte budget.
- [x] Injected late pages produce bounded diagnostics rather than hangs or crashes.
- [x] Salamander selected-zone and selected-group sustained preview produce audible output (peaks 0.454940587 and 0.896996021).

Required diagnostics: head/page hit/miss, read latency, queue depth, underrun/recovery, leased/resident/retired bytes.

## Phase 4 — Package Format v2

Outcome: metadata/TOC/heads open without whole payload; records are independently addressable and authenticated.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-400 | Package v2 schema and binary layout | Packed fixed header/TOC entries with unsigned 64-bit offsets/sizes/page identities; individually sealed manifest/instrument/index/head/page records. | `PackageV2.h/.cpp` | Normal and sparse 5 GiB layouts open; identity/AAD and bounds matrices pass. | `artifacts/large-instrument-streaming/phase-4/package-v2-record-storage.md` |
| [x] | STR-401 | Bounded package record reader and crypto API | Metadata-only TOC open plus exact one-record range read/auth/decrypt/checksum with 64 KiB record ceiling and precise failures. | `PackageV2.h/.cpp`, `PackageV2Tests.cpp` | Auth corruption, checksum sabotage, cancellation, truncation, duplicate, and oversize cases pass. | Same Phase 4 artifact. |
| [x] | STR-402 | Package page provider with corruption/cancel tests | Package-backed common data source retains immutable package TOC, maps head/page identities, uses shared intent scheduler, and publishes only verified float pages. | `SampleDataSource.h/.cpp`, package tests | Production voice renders head→page; cancel/corrupt page stays unpublished and prior page remains ready. | Same Phase 4 artifact. |
| [x] | STR-403 | Compatibility matrix and reader dispatch policy | Magic dispatches v2 bounded reader or v1 resident reader under a strict 64 MiB ceiling; oversized v1 requires re-export and no path rewrites on load. | `PackageReaderDispatch.h/.cpp`, package tests | Small generated v1 opens unchanged; sparse oversized v1 rejects before legacy read with one migration finding. | Same Phase 4 artifact. |

### Phase 4 acceptance gate

- [x] Opening v2 reads only header, metadata, TOC, and configured heads.
- [x] Any page can be authenticated by range without reading unrelated audio.
- [x] A corrupted page fails locally and cannot expose unauthenticated bytes.
- [x] Package offsets and tests cover payloads larger than 4 GiB.
- [x] Small v1 corpus behavior remains deterministic.

## Phase 5 — Streaming Export

Outcome: bounded chunk conversion/sealing writes a verified staging package and atomically publishes it.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-500 | Incremental compiler and package writer | Compiled float payload ranges become 16 KiB heads/64 KiB pages via lazy record loaders; writer holds one plaintext/sealed record at a time. | `PackageV2StreamingExport.h/.cpp`, `PackageV2.cpp`, tests | Six-record export peaks at 64 plaintext and 120 sealed bytes in fixture; 1 GiB plan remains 64-bit and record-bounded. | `artifacts/large-instrument-streaming/phase-5/streaming-export.md` |
| [x] | STR-501 | Crash-safe staged write and atomic publication | Writes fixed header + chunked TOC placeholder, appends records, patches metadata, verifies stage, then replace-existing/write-through publish. | `PackageV2.cpp` | Verified result has no `.stage`; canceled result has neither stage nor output. | Same Phase 5 artifact. |
| [x] | STR-502 | Responsive deterministic export cancellation | Progress exposes stage/record/bytes; cancellation checked before stage, each TOC chunk, record load/seal/write boundary, and verification. | `PackageV2.h/.cpp`, package tests | 1 GiB sparse plan cancellation cleans stage in 2.3 s on this Debug filesystem path, before any audio record buffer is loaded. | Same Phase 5 artifact. |
| [x] | STR-503 | Streaming verification pass and export report | Reopen header/TOC, sample-auth first/last records, report peaks, processed/verification bytes, durations and throughput without full second read. | `PackageV2.h/.cpp`, package tests | Verification reads 1,040 bytes vs package total and reports per-stage/total metrics. | Same Phase 5 artifact. |

### Phase 5 acceptance gate

- [x] Peak export memory stays within the configured bounded-buffer target.
- [x] No buffer scales with total package audio size.
- [x] Canceled and failed exports leave no publishable partial package.
- [x] The Salamander package exported at 2,631,961,513 bytes/40,865 records and passed bounded structural verification.

Required metrics: peak bytes; bytes read/converted/sealed/written; stage durations/throughput; cancellation latency.

## Phase 6 — Deferred Plug-in Activation

Outcome: explicit metadata/head/playable/active/degraded/failed stages progress automatically without UI/audio blocking.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-600 | Typed package-session lifecycle and UI projection | Separate metadata acceptance, source open, heads, model, pending activation, callback cutover; terminal/retryable failures. | `DeferredPackageSession.*`, `PerformancePackage.h` | Typed lifecycle and shared status projection matrix pass. | `artifacts/large-instrument-streaming/phase-6/deferred-package-activation.md` |
| [x] | STR-601 | Live continuation metadata-ready → playable | Retain path/TOC/metadata/cancel ID, prepare package source heads async, build model when admitted/ready. | `DeferredPackageSession.*`, package v2 tests | Actual package-backed source advances automatically from metadata through bounded heads to a common render model. | Same Phase 6 artifact. |
| [x] | STR-602 | Deferred activation with generation retention | Exchange only prepared token/pointer at block boundary; retain old sources/pages; audible only after callback cutover. | `DeferredPackageSession.*`, `SamplerPlaybackContext` integration tests | Active is published only after callback cutover; old voice survives replacement and retires safely. | Same Phase 6 artifact. |
| [x] | STR-603 | Lifecycle/recall/replacement stress coverage | Cancel on close/replacement; nonblocking host locator restore; missing/incompatible recovery guidance. | `DeferredPackageSession.*`, `PackageReaderDispatch.*`, package v2 tests | 100 replacement/cancels in 278 µs; locator request in 6 µs without resolver I/O; degraded and v1 re-export guidance covered. Final lifecycle, host-state, and realtime matrices pass. | Same Phase 6 artifact plus Phase 7 soak/host evidence. |
| [x] | STR-604 | Plug-in/standalone status and recovery parity | Separate metadata/head/cache/activation progress and actionable source/page failure categories. | `PerformancePackage.h`, `PluginEditor.cpp`, `MainComponent.cpp`, package v2 tests | Both production surfaces compile against one formatter; mapping/projection assertions pass. | Same Phase 6 artifact. |

### Phase 6 acceptance gate

- [x] Metadata appears quickly without claiming playable readiness.
- [x] The Salamander package progresses automatically to audible activation in standalone, plug-in, and restored-host paths.
- [x] Cancel, close, replacement, and host recall never block audio or UI threads in deterministic production-path stress coverage.
- [x] Old-generation voices finish safely across package replacement.
- [x] Plug-in and standalone lifecycle/status behavior match.

## Phase 7 — Qualification and Release

Outcome: large-instrument import through sustained playback meets bounded memory, realtime, integrity, and lifecycle gates.

| Status | Package | Deliverable | Implementation plan / notes | Files changed | Verification / result | Evidence / blocker / risk |
|---|---|---|---|---|---|---|
| [x] | STR-700 | CI-safe large-scale fixture toolkit | Sparse/generated >4 GiB, 641+ source, 1,704+ route, long/cache-pressure fixtures plus small legal corruption fixtures. | `LargeInstrumentFixtures.h`, PackageV2 and worker tests | 641/1,704 scale, 75-second source, 1,024-page pressure order, sparse 5 GiB package/RF64, 1 GiB cancellation, and legal corruption fixtures pass without retained large binaries. | `phase-7/package-corruption-matrix.md` |
| [x] | STR-701 | Signed Salamander qualification report and traces | Run import/preview/prepare/export/open/head/activate/first-note/sustain matrix with warm/first-open/constrained storage metrics. | `LargeInstrumentQualification.cpp` | Signed Release PASS: actual 641 WAVs/1,704 zones, bounded preview/full prep/export/open/playback/cancellation, 225,513,472-byte peak working set. Controlled OS cache flush was unavailable and is explicitly not claimed. | `phase-7/accurate-salamander-qualification.md` |
| [x] | STR-702 | Zero-violation realtime/lifecycle soak | Exercise notes/pedals/xfade/RR/replacement/recall/editor/shutdown with I/O/corruption/cancel/cache faults. | Production and test matrices | Debug/Release realtime guard passes; full semantic/lifecycle matrix and actual package host playback/recall pass with zero realtime violations. | `phase-7/realtime-lifecycle-soak.md`, `host-qualification.md` |
| [x] | STR-703 | Support matrix, migration notes, operational diagnostics | Document v1/v2, re-export, budgets, recovery, support bundles, release/architecture notes. | README and `docs/*large-instrument*`, package operator/compatibility/architecture docs | Support limits, recovery, readiness, host restore, diagnostics, v1 migration, and qualification results are current and reviewed. | `docs/large-instrument-streaming-support.md`, `phase-7/release-checklist.md` |
| [x] | STR-704 | Release checklist and source-audit evidence | Audit/remove whole-payload, payload-copy, full-corpus, synthetic-production, misleading-readiness bridges except intentional bounded resident path. | Package reader/writer/export, prepared playback, UI readiness | Unused reader bridges removed; v2 is record-bounded; v1/artwork/resident exceptions have enforced ceilings; all 34 packages and 38 acceptance items audited. | `phase-7/source-pattern-audit.md`, `release-checklist.md` |

### Phase 7 release gate

- [x] Salamander selected-zone preview is responsive and audible.
- [x] Full-draft preparation is bounded and reaches playable readiness.
- [x] v2 export completes within the approved memory ceiling.
- [x] Plug-in load reaches audible activation without UI or audio-thread stalls.
- [x] No realtime-guard, corruption, lifetime, or unbounded-residency finding remains.
- [x] Compatibility and recovery documentation is approved.

### Phase 7 evidence

- [x] V7.1 End-to-end timing/RSS report (`accurate-salamander-qualification.md`).
- [x] V7.2 Realtime and lifecycle soak (`realtime-lifecycle-soak.md`).
- [x] V7.3 Package corruption matrix (`package-corruption-matrix.md`).
- [x] V7.4 Host qualification report (`host-qualification.md`).
- [x] V7.5 Source-pattern audit (`source-pattern-audit.md`).

## Iteration-level decisions and controls

- D-01: retain an intentional small-instrument resident path behind the common source contract.
- D-02: start with configurable/evolvable 16 KiB heads and 64 KiB pages.
- D-03: package v2 uses independently authenticated records.
- D-04: no new codec, resampling, DSP, or articulation semantics.
- D-05: memory/responsiveness budgets are admission policy.
- Sequence: Phase 0; Phases 1–3; Phases 4–6; incremental Phase 7; final release gate last.
- Change discipline: keep resident baseline green, introduce typed seams before removal, separate callback and format changes, never weaken tests, capture gate evidence.

## Final inventory audit

- [x] All 34 STR packages above audited against the original HTML.
- [x] Every deliverable audited (34/34).
- [x] Every phase outcome audited (8/8).
- [x] Every acceptance checkbox audited and verified `checked` in HTML (38/38).
- [x] Every named evidence item audited.
- [x] D-01 through D-05 audited and preserved.
- [x] Final completion condition demonstrated on the Accurate Salamander Grand Piano.
