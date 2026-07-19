# Mini Sprint 4.6 — Route Normalization And Shell Cutover Evidence

Completed July 19, 2026.

## Outcome

The standalone shell and editor-closed plug-in now render through the same `drs_sampler_core` boundary. `PluginProcessor` owns shell I/O, bounded event translation, message-owned model construction, two isolated playback contexts, activation exchange, and primitive diagnostics; it no longer owns sampler voices, mixing DSP, fixture manifest traversal, or a mutable reference-sample cache.

## Implemented boundary

- `SamplerRenderModelBuildOptions` adapts selected zone/articulation, selected-zone audition, runtime note offset, and fixed velocity while the model is built off audio.
- Full payload topology is validated before route filtering. Empty selections and invalid note/velocity routing values are rejected before activation.
- UI events are translated into fixed-capacity Preview or Performance event blocks. Host MIDI is translated into the Performance block with its exact sample offset.
- `Processor::processBlock` clears the shell buffer, performs two additive calls—Performance then Preview—and records primitive results. It performs no file, path, decode, string, manifest, sample-map, voice-vector, interpolation, envelope, loop, or mixing work.
- Preview selection changes are detected independently of document revision so a newly selected zone receives a new message-owned normalized model.
- Playback-context diagnostic snapshots use coherent atomic publication for active activation, voice counts, and counters. Pending activation identity is read with a stable slot check.
- No activation is treated as no route and produces deterministic silence. The old callback fallback was removed rather than retained as a second production renderer.

## Parity and cutover evidence

- `drs.sprint4.voice_kernel` retains vectors captured from the removed legacy `renderBlockRange` formula, including center-pan, unity-pitch, velocity/gain, channel, final-frame, interpolation, and partition behavior.
- `drs.sprint4.shell_cutover` covers headless standalone, editor-closed plug-in, no-route silence, authored Preview audition, Preview/Performance isolation, exact host-MIDI offset, immutable Performance payload replacement, matching output, and matching diagnostics.
- Static source scan found no `ActiveRenderVoice`, `renderBlockRange`, `RealtimeRenderRoute`, reference playback cache, or fixture-specific resolver symbols in `PluginProcessor`.
- The production callback contains one renderer path only; no legacy/core dual rendering remains.

## Validation

Debug aggregate build:

```text
cmake --build build/vs2022-debug --target drs_all_tests --config Debug -j 4
Result: PASS
```

Full Debug CTest matrix:

```text
ctest --test-dir build/vs2022-debug -C Debug --output-on-failure
Result: 47/47 passed
```

Focused Sprint 4, entry-gate, and realtime matrix:

```text
ctest --test-dir build/vs2022-debug -C Debug -R "drs\.(sprint4|sprint4_entry|phase1\.realtime_safety)" --output-on-failure
Result: 12/12 passed
```

## Exit decision

Mini Sprint 4.6 exit criteria are met. `PluginProcessor` contains no sampler voice or mixing implementation and no fixture-specific callback route. Both shells invoke the same product-owned renderer, so Mini Sprint 4.7 may proceed.
