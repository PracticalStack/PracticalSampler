# Phase 1 Runtime Counters

This note captures the sixth Sprint 3 slice: the first observability counters for the streaming runtime.

## Current scope

Phase 1 now records:

- page misses when voices reach streamed data before the requested page is ready
- head usage counts plus cumulative head frames and bytes
- background read latency totals, averages, maxima, and the most recent sample
- active and peak voice counts inside the streaming runtime
- purge-pass, dormant-purge, last-purge, and cumulative eviction activity

The counters currently live in `RuntimeStreamingServiceMetrics`, with the voice runtime reporting head usage, misses, and active-voice lifecycle into that shared snapshot.

## Why this shape works for Phase 1

The goal is not to build a full diagnostics UI yet. The goal is to make the streaming runtime measurable enough that later host integration and regression debugging can start from evidence instead of guesswork.

That means the counters must be:

- cheap to sample from tests and future diagnostics surfaces
- explicit about whether stress came from cache pressure, voice pressure, or read latency
- stable enough that CI can validate directionally correct behavior during playback and idle recovery

## Validation

`drs_phase1_runtime_counters_tests` now proves:

- routed voices push active and peak voice counts upward during stress playback
- prefetch-head playback records head usage before the runtime stalls on streamed pages
- page misses increase when voices wait for streamed pages
- synthetic background read latency is captured in average and max counters
- idle recovery records dormant purge activity and trims resident pages back to the reviewed budget

The shared `drs_phase1_pipeline_report` artifact now also includes a `runtimeCounters` section so CI captures one benchmark-style observability snapshot alongside the rest of the Phase 1 runtime checks.
