# Mini Sprint 4.8 — Integration Hardening Evidence

Completed July 19, 2026.

## Outcome

Sprint 4's shared sampler core is integrated, real-time guarded, deterministic, and isolated across
Preview and Performance. The plug-in processor contains no competing sampler implementation or
reference-sample callback cache. Fresh Debug and Release evidence is green.

## Cleanup and diagnostics

- Removed the obsolete reference-cache count/warmup/load diagnostics and dynamic voice-capacity
  growth diagnostic that belonged to the deleted processor renderer.
- Kept compatibility helpers that remain outside the callback, including authoring waveform import
  and message-owned immediate Preview preparation.
- Extended the coherent odd/even primitive publication with stable context identity, last/max render
  time, peak active/releasing voices, steals, core and event-block drops, and producer queue drops.
- Counted shell event-block overflow separately from context-level event rejection and producer note
  overflow so load failures remain attributable.

Static callback source review found no legacy `ActiveRenderVoice`, `RealtimeRenderRoute`,
`renderBlockRange`, shell mixing function, reference sample map, or fixture resolver in the processor.

## Executable hardening

`drs.sprint4_entry.realtime_guard` now runs:

- all 12 EG4 negative injections through the production callback;
- 44.1 and 48 kHz at block sizes 32, 64, 128, 256, 512, and 1024;
- 24 active Performance voices plus 24 active Preview voices and 128 combined valid events;
- zero-drop/zero-guard assertions at every clean matrix point; and
- deliberate fixed-pool, event-block, and producer-queue pressure proving steal/drop diagnostics.

`drs.sprint4.concurrency_soak` runs 5,000 simultaneous 64-sample callback blocks plus 32 retirement
blocks while a message owner churns both context activations and a UI-style reader polls immutable
diagnostics. It proves distinct context identity, coherent monotonic publication, Preview/Performance
voice activity and release, activation replacement, retired-payload reclamation, and zero prohibited
audio-thread operations.

The Linux Clang ThreadSanitizer workflow builds and runs both the entry diagnostics regression and
the Sprint 4 concurrency soak with `DRS_ENABLE_THREAD_SANITIZER=ON`. ThreadSanitizer execution is a CI
responsibility and was not claimed as a local Windows result.

## Fresh validation record

| Validation | Result | Evidence |
| --- | --- | --- |
| Debug configure | Passed | New `build/sprint4-closure-debug` Ninja tree; MSVC 19.44.35228. |
| Debug aggregate | Passed | `drs_all_tests`; 749 effective build steps after configure. |
| Debug full CTest | **49/49 passed** | 157.16 s; soak 7.01 s, guard matrix 29.59 s, shell cutover 6.90 s, offline renderer 0.06 s. |
| Release configure | Passed | New `build/sprint4-closure-release` Ninja tree. |
| Release VST3 | Passed | Bundle and `moduleinfo.json` generated under `app/drs_plugin_bundle_artefacts/Release/VST3`. |
| Release supported set | **3/3 passed** | 8.03 s: VST3 smoke 3.15 s, offline conformance 0.16 s, benchmark scene 4.70 s. |

The first narrow Release run built the smoke executable without its VST3 scan fixture and therefore
failed discovery. `drs_phase0_smoke_tests` now explicitly depends on `DecentRhapsodyStudioPlugin`;
after the bundle was built, the unchanged smoke test passed. This was a build-graph defect, not an
audio or product-runtime defect.

## Artifacts and benchmark

- Reviewed offline baselines: `tests/baselines/sprint4-offline-render-baselines.txt`.
- Offline mismatch policy: no artifact is written on success; JSON evidence is emitted only on a
  mismatch and never overwrites the reviewed baseline.
- Release VST3: `build/sprint4-closure-release/app/drs_plugin_bundle_artefacts/Release/VST3/Decent Rhapsody Studio.vst3`.
- Release benchmark report: `build/sprint4-closure-release/tests/phase1-benchmark-scene.json`.
- Benchmark result: ordinary playback 63,585 µs; three-voice moderate-polyphony scene 157,643 µs;
  average synthetic read latency 6,460 µs; load-profile switch 101,486 µs; all scenarios passed.

## Defects

One build dependency defect was found and corrected as described above. No open Sprint 4 renderer,
callback, deterministic-render, context-isolation, activation-lifetime, or diagnostic defect remains.
