# Tests

Phase 0 test work should start here with a minimal smoke-test harness for startup and runtime initialization.

The first goal is not broad coverage. It is a small, reliable signal that the shell and engine bootstrap do not immediately fail.

Current baseline:

- `drs_phase0_smoke_tests` exercises the product-owned engine facade and content resolver.
- The same executable instantiates the standalone shell component and plugin editor shell without launching the full app.
- `drs_phase1_runtime_contract_tests` validates the product-owned `.drinst` fixture contract plus negative fixtures.
- `drs_phase1_sample_import_tests` generates paired WAV and FLAC fixtures, imports them through the product-owned decoder seam, and checks normalized metadata plus failure reporting.
- `drs_phase1_compile_path_tests` imports the real reference WAV sources, compiles deterministic runtime artifacts in a temp directory, confirms the Sprint 1 loader can open the generated manifests, and rejects unsupported compile-policy inputs loudly.
- `drs_phase1_pipeline_report` writes a single JSON report for the reference corpus covering loader status, importer status, compile determinism, golden-file parity, and corruption checks.
- `drs_phase1_runtime_baseline_report` prints the first cold-vs-warm manifest-load baseline report for the reference instrument and writes `phase1-runtime-baseline.json` in the test build directory when run through CTest.
- `drs_phase1_runtime_baseline_guard` compares the generated baseline artifact against the checked-in snapshot and fails CI when static fields drift or timing moves beyond the reviewed tolerance window.
- `drs_phase1_runtime_fixture_tool` can verify or intentionally rewrite the checked-in Sprint 1 reference fixtures and baseline snapshot.
- `ctest --preset test-debug` and `ctest --preset test-release` are the supported local entry points after configuration and build.
