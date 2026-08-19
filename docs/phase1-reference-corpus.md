# Phase 1 Reference Corpus Plan

This note captures the Sprint 1 reference corpus plan for the Runtime MVP. The goal is to ensure every later runtime decision is exercised against named fixtures instead of ad hoc local content.

## Corpus tiers

### 1. Tiny open instrument

Status: ready in Sprint 1

Purpose:

- canonical loader fixture
- public demo artifact
- clean-machine smoke-test target

Current implementation:

- `content/runtime/phase1/reference-corpus/tiny-open-instrument/tiny-open-instrument.drsproj`
- `content/runtime/phase1/reference-corpus/tiny-open-instrument/tiny-open-instrument.drinst`
- `content/runtime/phase1/reference-corpus/tiny-open-instrument/tiny-open-instrument.drstrm`

This fixture uses the reusable native product sample fixtures in `content/samples/`, keeping the
reference corpus independent of any authoring-tool project layout.

Sprint 2 now also checks in a compiler-emitted prototype `.drstrm` descriptor for this fixture so the reference corpus captures deterministic payload offsets and page-table placeholders instead of only a placeholder container stub.

Sprint 5 now also checks in a package manifest for this fixture so contributors can review one product-owned artifact that names:

- the source sample inputs
- the checked-in runtime manifests
- the compiled stream descriptor
- the expected runtime shape needed for repeatable validation

Sprint 7 now also checks in a sealed package corpus for this fixture at:

- `content/runtime/phase1/reference-corpus/tiny-open-instrument/performance-package-corpus/index.json`

That corpus includes:

- one valid `.drpkg`
- truncated and tampered corruption cases
- wrong-version skew coverage
- missing-payload and checksum-mismatch loader failures

### 2. Medium internal streaming case

Status: planned

Purpose:

- first realistic multi-zone stream benchmark
- first content set that can expose bad prefetch defaults
- baseline for load time, RAM, and page-miss tracking

Expected entry point:

- tracked in `content/runtime/phase1/reference-corpus/index.json`

Target delivery:

- ready before Sprint 3 streaming work starts

### 3. Synthetic stress manifest

Status: planned

Purpose:

- stress voice counts without needing a huge licensed sample set
- exercise purge-policy and paging-path assumptions
- simulate wide zone counts and exaggerated stream offsets

Target delivery:

- ready before Sprint 3 or early in Sprint 3

## Validation vocabulary

Sprint 1 also locks the names of the core metrics that later benchmarks will capture.

### Load-path metrics

- cold load time
- warm load time
- manifest parse time
- source project resolution success
- compiled stream asset resolution success

### Runtime-shape metrics

- macro count
- articulation count
- group count
- zone count
- referenced sample count
- total prefetch bytes

### Later streaming metrics

These become active once the paging runtime exists:

- page misses
- read latency
- head-cache hit rate
- active voice count
- purge events
- steady-state RAM

## Validation rules

- every new runtime fixture must be named in the corpus index
- every fixture family needs at least one negative or corrupt counterpart eventually
- the tiny open instrument must remain small enough for fast smoke validation
- later larger fixtures must not replace the tiny fixture; they complement it

## Contributor package flow

The tiny open instrument is now also packaged as a checked-in package description at:

- `content/runtime/phase1/reference-corpus/tiny-open-instrument/package-manifest.json`

That file is intentionally generated from the canonical runtime loaders and stream metadata instead of being hand-maintained. The contributor-facing entry point is:

- `powershell -ExecutionPolicy Bypass -File .\tools\package-phase1-reference-instrument.ps1 -Mode Verify`

That wrapper:

- configures the Windows presets if needed
- builds `drs_phase1_runtime_fixture_tool`
- runs the same verification path that CI uses for the checked-in manifests and package metadata

If the reference package changes intentionally, refresh the checked-in package manifest with:

- `powershell -ExecutionPolicy Bypass -File .\tools\package-phase1-reference-instrument.ps1 -Mode Refresh`

That same refresh now also rewrites the checked-in sealed package corpus under `performance-package-corpus/`.

Refreshes should be followed by a targeted validation pass:

- `ctest --preset test-debug -R "drs.phase1.fixture_tool_verify|drs.phase1.performance_package|drs.phase1.performance_package_loader|drs.phase1.performance_package_host_validation|drs.phase1.runtime_contract|drs.phase1.macro_bridge|drs.phase0.smoke" --output-on-failure`
- open the standalone app and confirm `Load Default` and `Load Lead Demo` both produce the expected playable state from the Sprint 5 performance surface

## Sprint 1 outcome

The repo now has:

- a product-owned corpus index
- a named public reference instrument
- named negative fixtures for missing-default and missing-sample validation
- a corrupt malformed-JSON fixture for parse-failure validation
- a clear placeholder for the medium and stress cases
- a stable metric vocabulary that later validation work can build on
- a generated package manifest that ties the public reference source content to the checked-in runtime artifact set
