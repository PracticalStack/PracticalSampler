# Phase 1 Benchmark Scene

This note captures Sprint 5 task `P1-504`: a named playback scene that exercises the Phase 1 milestone in one repeatable pass.

## Scene artifact

The checked-in scene contract lives at:

- `content/runtime/phase1/benchmark-scenes/reference-playback-scene.json`

The scene is intentionally small and reviewable. It names one reference instrument, one preset-state fixture, and four required playback behaviors:

- ordinary note playback on the default sustain path
- moderate three-voice polyphony against the streaming runtime
- preset restore, export, and reload through the standalone shell seam
- live load-profile downgrade from `performance` to `eco` while playback remains valid

## Runner

The product-owned runner is:

- `drs_phase1_benchmark_scene`

When run through CTest, it writes:

- `phase1-benchmark-scene.json`

in the active test build directory.

For direct local runs, use:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run-phase1-benchmark-scene.ps1
```

That wrapper configures the Windows preset, builds the benchmark target, runs the checked-in reference scene, and writes a JSON report under the matching build tree.

## Report shape

The report is designed for comparison between runs rather than exact audio-performance certification. It records:

- ordinary playback zone selection and preview duration
- moderate polyphony page waits, peak active voices, read counts, and latency metrics
- preset reload success plus restored macro values
- load-profile downgrade success, lease preservation, purge results, and switch duration

The report passes only when all four scene sections succeed.

## Validation

The minimum validation flow for this slice is:

- `ctest --preset test-debug -R "drs.phase1.benchmark_scene" --output-on-failure`
- `powershell -ExecutionPolicy Bypass -File .\tools\run-phase1-benchmark-scene.ps1`

For manual review, compare the generated JSON report between runs and confirm that:

- the same scene id is reported
- all four sections still pass
- routed zones and restored macro values stay stable
- polyphony and load-profile metrics remain directionally comparable instead of silently disappearing
