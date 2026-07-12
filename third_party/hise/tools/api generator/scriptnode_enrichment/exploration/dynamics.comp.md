# dynamics.comp - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/DynamicsNode.h:80-289`
**Base class:** `dynamics_wrapper<chunkware_simple::SimpleComp>`, inherits `HiseDspBase` + `display_buffer_base<true>`
**Classification:** audio_processor

## Signal Path

Input audio -> peak detection (per-frame, stereo or mono) -> chunkware SimpleComp algorithm (threshold/attack/release/ratio) -> gain-reduced audio output. Simultaneously outputs inverse gain reduction (1.0 - GR) as normalised modulation signal (0..1).

Processing is frame-based: `process()` converts block to stereo frames via `forwardToFrameStereo()`, then calls `processFrame()` per sample. After block completes, modulation value is updated once.

For stereo: processes both channels through `obj.process(L, R)`.
For mono: duplicates the sample to both channels for the compressor, takes back channel 0.

## Gap Answers

### signal-path-processing: How does the compressor process audio?

`process()` uses `forwardToFrameStereo()` to iterate per-sample. `processFrame()` extracts stereo or mono samples, calls `obj.process(values[0], values[1])` on the chunkware SimpleComp instance. The chunkware library handles envelope detection, gain computation, and gain application internally. After all frames, `updateModValue()` stores `1.0 - obj.getGainReduction()` as the modulation output.

### sidechain-modes: What are the 3 Sidechain parameter values?

Enum `SidechainMode`: Disabled(0), Original(1), Sidechain(2).
- **Disabled:** Default. Self-sidechaining -- the compressor detects from its own input.
- **Original:** Same as Disabled -- processes with `obj.process(L, R)` without external sidechain.
- **Sidechain:** Uses external sidechain input. For stereo, channels are split in half: channels 0-1 are audio, channels 2-3 are sidechain (peak of abs(ch2), abs(ch3)). For mono input (2 channels in sidechain mode), channel 0 is audio, channel 1 is sidechain. Calls `obj.process(L, R, sidechainValue)`.

### ratio-behaviour: How is the Ratio parameter applied?

`setRatio()` inverts the user value: `ratio = (v != 0.0) ? 1.0 / v : 1.0`. So a user-facing ratio of 4:1 passes 0.25 to the chunkware compressor. This matches chunkware's convention where ratio < 1 means compression.

### modulation-output: Does the node actually have modulation output despite empty ModulationTargets?

YES. `isNormalisedModulation()` returns true. `handleModulation()` returns the `modValue` via `getChangedValue()`. The modulation value is `jlimit(0.0, 1.0, 1.0 - obj.getGainReduction())` -- inverse gain reduction. At no compression (GR=0), output is 1.0. At full compression, output approaches 0.0.

### display-buffer-content: What does the DisplayBuffer show?

The display buffer receives the same value as the modulation output: `updateBuffer(mv, numSamples)` where `mv = 1.0 - obj.getGainReduction()`. Shows gain reduction over time.

### description-accuracy: Is "ducking amount as modulation signal" accurate?

Partially. The modulation signal is the *inverse* gain reduction (1.0 - GR), not the ducking amount directly. Higher values mean less compression. The term "ducking" is imprecise for a general-purpose compressor.

## Parameters

- **Threshhold:** Sets compressor threshold via `obj.setThresh(v)`. Range -100..0 dB.
- **Attack:** Sets attack time via `obj.setAttack(v)`. Range 0..250 ms.
- **Release:** Sets release time via `obj.setRelease(v)`. Range 0..250 ms.
- **Ratio:** Inverted internally (1/v) before passing to chunkware. User value 4 = 4:1 compression. Default 1 = no compression.
- **Sidechain:** Selects sidechain source. Disabled/Original/Sidechain.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

## Notes

- comp, gate, and limiter share the identical `dynamics_wrapper<T>` template. Only the chunkware algorithm type differs.
- The "Threshhold" parameter ID has a typo (double 'h') -- this is in the original code.
- When Sidechain=Sidechain, the node expects double the channel count (e.g., 4 channels for stereo processing). The container must provide the extra channels.
- `updateOnFrame` flag prevents per-frame modulation updates during block processing; the modulation is updated once after the full block completes.
