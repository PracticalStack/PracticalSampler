# Curated DSP S0 Baseline Evidence

Task: DSP-00-02 and DSP-00-04  
Captured: July 30, 2026  
Profile: Windows 11 x64, Visual Studio 2022 17.14.34, MSVC 14.44.35207, Debug, Ninja,
48 kHz, stereo, 512 frames per block.

## Offline output baseline

The reviewed baseline is `tests/baselines/sprint4-offline-render-baselines.txt`. It uses a
quantized sample checksum plus peak, RMS, first/last non-zero frame, and voice lifecycle counters
with a `1e-6` tolerance. The zero-DSP renderer produces no effect processing, so these artifacts
are the compatibility reference before the graph path is introduced.

| Required S0 case | Reviewed scenario(s) | Evidence |
|---|---|---|
| No FX | `silence`, `mono-root-unity`, `partition-invariance` | `1c069e6843ea3a48`, `65a603aa17ed9008`, `96686e2ffdf192fa` |
| Authored-but-inert FX metadata | schema-4 runtime model has metadata-only `fxSlots`/`routingBuses`; the offline render snapshot has no DSP node or effect state | Existing FX cannot reach `SamplerRenderModel`; S1 migration must preserve this exact dry path |
| Non-zero group gain/pan | `grouped-mix-balance` | `509592d7c2e95f98` |
| Stereo | `stereo-channels` | `1a24bdfdb5f78903` |
| Preview and Performance lane isolation | `drs.phase1.realtime_safety` | Independent preallocated contexts and zero tracked callback violations |

The offline command was run twice against the checked-in baseline. Both passes reported 21
reviewed scenarios and identical partition coverage: 32/64/127/256/512/1024 frames.

## Callback and retention measurement

`drs_curated_dsp_s0_baseline_report` provides a reproducible no-DSP measurement. It prepares the
default published playback at 48 kHz / 512 frames, renders 256 callbacks, asserts audible output,
asserts zero prohibited audio-thread operations, and writes the measured callback and active/
retired payload values as JSON. The generated report is intentionally build-local because timings
are machine-specific.

Run it from a VS 2022 developer shell:

```powershell
cmake --build build/vs2022-debug --target drs_curated_dsp_s0_baseline_report
.\build\vs2022-debug\tests\drs_curated_dsp_s0_baseline_report.exe .\build\vs2022-debug\curated-dsp-s0-baseline.json
```

The ADR freezes the resulting policy rather than a machine-specific timing value: no-DSP added
overhead may consume at most 1% of a block period; any legal DSP graph may consume at most 50%.
Graph resources are rejected at the exact limits recorded in `curated-dsp-contract.md`.

### Captured result

The first capture reported 256 blocks, a 10,667 microsecond host deadline, a 62 microsecond last
callback, a 543 microsecond maximum callback, zero deadline/real-time-guard failures, 705,600
active prepared bytes, and zero retired bytes. The values are baseline evidence, not a promise
that the Debug build meets the later production headroom target.
