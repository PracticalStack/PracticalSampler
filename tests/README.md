# Tests

Phase 0 test work should start here with a minimal smoke-test harness for startup and runtime initialization.

The first goal is not broad coverage. It is a small, reliable signal that the shell and engine bootstrap do not immediately fail.

Current baseline:

- `drs_phase0_smoke_tests` exercises the product-owned engine facade and content resolver.
- The same executable instantiates the standalone shell component and plugin editor shell without launching the full app.
- `ctest --preset test-debug` and `ctest --preset test-release` are the supported local entry points after configuration and build.
