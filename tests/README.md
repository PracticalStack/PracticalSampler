# Tests

Phase 0 test work should start here with a minimal smoke-test harness for startup and runtime initialization.

The first goal is not broad coverage. It is a small, reliable signal that the shell and engine bootstrap do not immediately fail.

Current baseline:

- `drs_phase0_smoke_tests` exercises the product-owned engine facade and content resolver.
- The same executable instantiates the standalone shell component and plugin editor shell without launching the full app.
- `drs_phase1_runtime_contract_tests` validates the product-owned `.drinst` fixture contract plus negative fixtures.
- `drs_phase1_preset_state_tests` validates the Sprint 4 preset-state contract, golden recall fixtures, and rejection of leaked transient diagnostics.
- `drs_phase1_state_recall_tests` exercises standalone and plugin save/reload round-trips plus rejection of invalid restore payloads.
- `drs_phase1_diagnostics_tests` validates the Sprint 4 diagnostics snapshot, load-profile budget exposure, purge counters, and visible failure-state reporting.
- `drs_phase1_failure_handling_tests` validates graceful reporting for missing content, bad checksums, schema mismatch, and partially compiled artifacts while preserving the last known-good session state.
- `drs_phase1_macro_bridge_tests` validates standalone macro edits, plugin host-facing macro parameters, reference macro effects, preview-response behavior, and macro persistence across save/reload.
- `drs_phase1_sample_import_tests` generates paired WAV and FLAC fixtures, imports them through the product-owned decoder seam, and checks normalized metadata plus failure reporting.
- `drs_phase1_compile_path_tests` imports the real reference WAV sources, compiles deterministic runtime artifacts in a temp directory, confirms the Sprint 1 loader can open the generated manifests, and rejects unsupported compile-policy inputs loudly.
- `drs_phase1_pipeline_report` writes a single JSON report for the reference corpus covering loader, stream-reader, streaming-service, voice-runtime, note-routing, load-profile, runtime-counters, importer, compile-determinism, golden-file-parity, state-recall, macro-state compare, and error-handling status, plus a nightly summary block for load, play, state recall, and error handling.
- `drs_phase1_benchmark_scene` runs the checked-in reference playback scene and writes a single JSON report covering ordinary playback, moderate polyphony, preset reload, and load-profile switching.
- `drs_phase1_stream_reader_tests` loads the checked-in `.drstrm` artifact, verifies prefetch and page lookup math, and fails cleanly on checksum or offset corruption.
- `drs_phase1_streaming_service_tests` exercises the background page-read scheduler, cache hits, in-flight request coalescing, lease release, and cache-budget eviction behavior.
- `drs_phase1_voice_runtime_tests` exercises voice allocation, macro snapshots, wait-at-page-boundary behavior, polyphony cleanup, and stale-lease prevention.
- `drs_phase1_note_routing_tests` exercises default-articulation selection, explicit `lead` routing, velocity-layer selection, routed playback hand-off, and loud failures for unmapped triggers.
- `drs_phase1_load_profile_tests` exercises named `eco`/`balanced`/`performance` budgets, per-voice prefetch clamping, live profile downgrade behavior, dormant-page purge, and unknown-profile rejection.
- `drs_phase1_runtime_counters_tests` exercises page misses, head usage, read latency, active voice counts, and purge activity during stress playback and idle recovery.
- `drs_phase1_runtime_baseline_report` prints the first cold-vs-warm manifest-load baseline report for the reference instrument and writes `phase1-runtime-baseline.json` in the test build directory when run through CTest.
- `drs_phase1_runtime_baseline_guard` compares the generated baseline artifact against the checked-in snapshot and fails CI when static fields drift or timing moves beyond the reviewed tolerance window.
- `drs_phase1_runtime_fixture_tool` can verify or intentionally rewrite the checked-in Sprint 1 reference fixtures, package manifest, and baseline snapshot.
- `drs_wav_import_baseline_report` prints the historical synchronous WAV-import baseline for constructor, replace, restore, submit, full-batch, and memory-shape diagnostics, and writes `wav-import-baseline-report.json` in the test build directory when run through CTest. The reviewed July 31, 2026 snapshot lives at `validation/wav-import/sync-shell-baseline.json`.
- `drs_wav_import_ci_budget_tests` hard-gates zero-I/O constructor/import-submit/waveform-submit behavior plus reviewed large-batch snapshot and waveform-peak memory budgets, and reports the measured WAV-import and waveform-preview cancellation timings with build-aware tolerance context for CI logs.
- `drs_wav_import_host_validation_tests` hard-gates standalone startup against missing-local, removable-drive-like, and UNC-like sample-source paths, requiring zero sample-import I/O, `not-run` responsiveness state, and reviewed Debug startup/replace timing budgets before the REAPER matrix is consulted.
- `ctest --preset test-debug` and `ctest --preset test-release` are the supported local entry points after configuration and build.
