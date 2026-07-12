# dynamics.envelope_follower - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/DynamicsNode.h:300-437`
**Base class:** `display_buffer_base<true>` + `polyphonic_base`
**Classification:** audio_processor (with modulation output)

## Signal Path

Input audio -> peak detection (max of abs across all channels per frame) -> per-voice attack/release envelope follower -> modulation output (envelope value 0..1). If ProcessSignal is enabled, replaces all audio channels with the envelope value; otherwise audio passes through unmodified.

Processing is frame-based: `process()` routes to mono or stereo frame processing depending on channel count. `processFrame()` computes peak across all channels, runs the per-voice AttackRelease envelope, optionally writes envelope to output, and sets modulation value.

## Gap Answers

### signal-path-processing: How does the envelope follower process audio?

`processFrame()`:
1. Compute `input = max(abs(s))` across all channels
2. Run `envelopes.get().calculateValue(input)` -- per-voice AttackRelease envelope
3. If `processSignal` is true, replace all channel samples with the envelope `output` value
4. Set `modValue.setModValue(output)` for modulation output

The `process()` method delegates to mono or stereo frame converters based on channel count. After block processing, `handleModulation()` calls `updateBuffer()` for the display.

### description-empty: What does this node actually do?

Tracks the amplitude envelope of the input signal using a per-voice attack/release follower. Outputs the tracked envelope as a normalised modulation signal (0..1). Optionally replaces the audio output with the envelope value.

### process-signal-parameter: What does ProcessSignal do?

Two modes via `setParameterValueNames({"Off", "On"})`:
- **Off (0):** Analysis-only mode. Audio passes through unmodified. The envelope is tracked and output as modulation only.
- **On (1):** Replaces all audio channel samples with the envelope follower output value. The audio becomes the envelope shape.

### modulation-output: Does the node have modulation output?

YES. `isNormalisedModulation()` returns true. `handleModulation()` returns the envelope value via `modValue.getChangedValue(v)`. Output is 0..1 (the raw envelope follower output).

### display-buffer-content: What does the DisplayBuffer show?

The display buffer receives the modulation value: `updateBuffer(modValue.getModValue(), lastNumSamples)`. Shows the tracked envelope waveform over time.

## Polyphonic Behaviour

Uses `PolyData<EnvelopeFollower::AttackRelease, NV> envelopes` for per-voice envelope state. Each voice maintains its own attack/release envelope follower. `envelopes.get()` retrieves the current voice's state during processing. All voice envelopes share the same Attack and Release time settings (set via `for(auto& envelope: envelopes)` loop).

`reset()` resets all voice envelopes via the same loop pattern.

Constructor passes `polyphonic_base(getStaticId(), false)` -- the `false` means it does not require the voice manager for reset (it handles reset internally).

## Parameters

- **Attack:** Sets attack time for all voice envelopes via `setAttackDouble(v)`. Range 0..1000 ms (longer range than comp/gate/limiter).
- **Release:** Sets release time for all voice envelopes via `setReleaseDouble(v)`. Range 0..1000 ms.
- **ProcessSignal:** Boolean toggle. Off = pass-through audio, On = replace audio with envelope value.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: [{"parameter": "voice count", "impact": "linear", "note": "Per-voice envelope calculation via PolyData iteration"}]

## Notes

- This is the only polyphonic node in the dynamics factory.
- Unlike comp/gate/limiter, this node does NOT use the chunkware library. It uses HISE's own `EnvelopeFollower::AttackRelease` class.
- The envelope follower tracks amplitude (peak detection), not RMS.
- When ProcessSignal=On, the audio output becomes a DC-like signal following the envelope -- useful for driving other parameters but not for audio output.
- The `lastNumSamples` member tracks block size for display buffer updates in `handleModulation()`.
