# NoiseGrainPlayer - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/NoiseGrainPlayer.h`, `hi_core/hi_modules/effects/fx/NoiseGrainPlayer.cpp`
**Base class:** `VoiceEffectProcessor`, `AudioSampleProcessor`, `MultiChannelAudioBuffer::Listener`

## Signal Path

Audio file -> SiTraNoConverter (offline FFT analysis) -> noise grain collection (pre-computed).

Per voice render: input voice signal -> overlap fader dry gain -> add grain output scaled by wet gain -> output.

The module pre-analyses the loaded audio file using SiTraNoConverter's sinusoidal-transient-noise decomposition. It extracts the noise component as individual Hann-windowed FFT frames ("noise grains"). At runtime, grains are selected by the modulated Position parameter, overlapped 4x (hop = grainLength/4), and added to the input signal using an overlap fader crossfade.

## Gap Answers

### signal-path-order: What is the processing order in the voice render method?

In `applyEffect()`:
1. Compute modulated position: `t = jlimit(0, 1, Position) * GainModValue + BipolarModValue`
2. Compute overlap fader gains from Mix: `g1 = overlap::getFadeValue<0>(2, mix)` (dry), `g2 = overlap::getFadeValue<1>(2, mix)` (wet)
3. Scale the input buffer by g1: `b.applyGain(startSample, numSamples, g1)`
4. Call `voiceState[voiceIndex].render(b, ..., t, g2)` which **adds** grain samples to the buffer (using `addSample`), scaled by g2

The input voice signal is NOT replaced - it is scaled down by the dry fader and the grain output is added on top. At Mix=1.0, g1=0 (dry silent) and g2=1.0 (grain at full), so the output is pure grain. At Mix=0.0, g1=1.0 and g2=0.0 (pure dry passthrough). Around Mix=0.5 both are at full volume (overlap region).

### grain-algorithm: How does the granular playback work?

**Offline stage (recalculate):** The loaded audio file is processed by `SiTraNoConverter` with `fastFFTOrder = 7 + grainSize` (so GrainSize 1-6 maps to FFT orders 8-13, producing FFT sizes 256-8192). The converter performs STFT with a Hann window, applies median filtering to separate sinusoidal/transient/noise components, then inverse-FFTs the noise component back to time domain. Each IFFT frame becomes one grain, Hann-windowed and scaled by 1/3 during resynthesis. The grains are stored in `noiseGrains` (a `std::vector<AudioSampleBuffer>`).

**Runtime stage (VoiceState::render):** Each voice tracks an `uptime` counter. The hop index is `(int)(uptime / hopSize)` where `hopSize = grainLength / 4`. When the hop index advances, a new grain is started in the first available slot (8 slots per voice). The grain index is `roundToInt(tableIndex * (numGrains-1))`, with random jitter (+/-5) if the same grain would repeat consecutively.

**Grain::apply:** Per-sample processing with linear interpolation. The grain plays through its buffer from start to end at `uptimeDelta = hostSampleRate / fileSampleRate`. When `uptime >= grainLength`, the grain deactivates. The grain sample is **added** to the output buffer (not replacing).

**Window function:** Hann. Applied during the SiTraNoConverter IFFT stage (each grain is Hann-windowed before storage). No additional window is applied at runtime.

**Overlap:** 4x overlap (hop = grainLength/4). With 8 grain slots, this supports continuous playback with headroom for timing variations.

### white-noise-meaning: Audio-domain noise mixing vs position-domain randomization?

Position-domain randomization. In `Grain::apply()`, the per-sample read position is: `tUptime = uptime + noiseAmp * (random - 0.5)` where `noiseAmp = whiteNoise * 10.0`. This adds a random bipolar offset to the read position within the grain, with the result wrapped around the grain boundary via `hmath::wrap()`. At WhiteNoise=0, grains play back faithfully. At WhiteNoise=1, the read position jitters by up to +/-5 samples, randomizing the phase relationships and creating a noisier texture.

The module description ("blends an audio file with white noise") is inaccurate. The WhiteNoise parameter does not add audio-domain white noise - it randomizes the grain read position, producing a noise-like effect through phase scrambling.

### dual-modchain-interaction: How are gain and offset modulation combined?

In `applyEffect()`:
```
t = jlimit(0, 1, tableIndex)   // base Position parameter
tg = modChains[PositionChain].getOneModulationValue(startSample)     // gain mod
tb = modChains[PositionBipolar].getOneModulationValue(startSample)   // offset mod
t *= tg    // multiply first
t += tb    // then add
```

Order: `finalPosition = (Position * GainMod) + OffsetMod`. Both are per-block (one value per render call via `getOneModulationValue`), not per-sample.

### mix-crossfade-curve: What crossfade curve does the Mix parameter use?

The overlap fader from `scriptnode::faders::overlap` with 2 elements:
- Index 0 (dry): `jlimit(0, 1, 2 - 2*mix)` - full at mix<=0.5, fades to 0 at mix=1.0
- Index 1 (wet): `jlimit(0, 1, 2*mix)` - 0 at mix=0, fades to full at mix>=0.5

This is a linear overlap crossfade, NOT equal-power. Both signals are at full volume around Mix=0.5 (the "overlap" region where total gain can exceed unity). Mix=0 is fully dry, Mix=1 is fully wet.

### grain-size-behavior: How does grain size interact with file length?

GrainSize (1-6) maps to FFT orders 8-13 via `fastFFTOrder = 7 + grainSize`, producing FFT sizes 256-8192 samples. The SiTraNoConverter processes the entire audio file through STFT, producing as many grains as there are FFT frames. The number of grains depends on file length: `numGrains = floor((paddedLength - fftSize) / hopSize + 1)`.

If the file is shorter than one FFT frame, no grains are produced and `hopSize` is set to 0, which causes `VoiceState::render()` to early-exit (the `if(hopSize != 0.0)` guard). Grains never extend past the file boundary because the SiTraNoConverter zero-pads the input. Larger grain sizes produce fewer, longer grains with lower frequency resolution in the noise separation.

### audio-sample-usage: How is AudioSampleProcessor used?

The audio file is loaded via `AudioSampleProcessor` and accessed through `getBuffer()` and `getAudioSampleBuffer()`. The entire file is loaded into memory and processed by SiTraNoConverter when the buffer changes (via the `MultiChannelAudioBuffer::Listener` callbacks `bufferWasLoaded` and `bufferWasModified`, both calling `recalculate()`).

The file can be changed at runtime - `recalculate()` uses `killVoicesAndCall()` to safely reprocess on the sample loading thread, swapping the grain collection under a write lock.

The file read position is NOT affected by voice pitch. `uptimeDelta = hostSampleRate / fileSampleRate` is a fixed ratio based on sample rates, not pitch. All voices play grains at the same rate regardless of note number.

### per-voice-performance: CPU cost per voice?

**Grain rendering is per-sample.** Each active grain iterates sample-by-sample with linear interpolation (2 sample lookups + lerp + random number generation per sample). With 4x overlap, typically 4 grains are active simultaneously per voice, so cost is roughly 4x per-sample interpolation per voice.

The random number generation (`r.nextDouble()`) runs per sample per active grain when WhiteNoise > 0. At WhiteNoise=0, the random offset is zero but the RNG still runs (the multiplication by noiseAmp=0 zeroes it out, but the call is not skipped).

Grain size does NOT significantly affect per-voice CPU cost - larger grains mean fewer grain starts per second but the same total sample processing. The offline SiTraNoConverter analysis cost increases with smaller grain sizes (more FFT frames) but this runs asynchronously on the sample loading thread.

## Processing Chain Detail

1. **Offline: SiTraNoConverter analysis** (on sample loading thread, once per file/grainSize change): FFT decomposition -> median filtering -> noise extraction -> IFFT to time-domain grains. CPU: high (FFT-based), but asynchronous and one-time.
2. **Modulation read** (per-block): Position gain and bipolar mod chain values read via `getOneModulationValue()`.
3. **Overlap fader calculation** (per-block): Dry/wet gains computed from Mix parameter.
4. **Dry signal scaling** (per-block): Input buffer scaled by dry gain g1.
5. **Grain scheduling** (per-block): Hop index check, grain selection by position, grain start in available slot.
6. **Grain rendering** (per-sample): Linear interpolation with optional position noise, added to output buffer scaled by wet gain g2.

## Modulation Points

- **Table Index Modulation** (chainIndex 0, GainMode): Multiplicative scaling of Position. Applied per-block before offset.
- **Table Index Bipolar** (chainIndex 1, OffsetMode): Bipolar additive offset to Position. Applied per-block after gain scaling.

Both chains use `getOneModulationValue()` - one value per render call, not sample-accurate.

## Conditional Behavior

No mode switches or parameter-gated paths. The processing chain is fixed. The only conditional is `if(hopSize != 0.0)` in `VoiceState::render()` which guards against no grains being available (empty file or file shorter than one FFT frame).

Grain index jitter: if the selected grain index equals the last started grain index, a random offset of +/-5 is applied to avoid repeating the same grain consecutively.

## Interface Usage

**AudioSampleProcessor:** Provides the source audio file. The full file buffer is accessed via `getAudioSampleBuffer()` and passed to `SiTraNoConverter::process()`. The buffer's sample rate (`getBuffer().sampleRate`) is used to compute `uptimeDelta` for playback rate conversion. The buffer's `sendDisplayIndexMessage()` is called in `applyEffect()` for the last-started voice to update the UI waveform display position.

**MultiChannelAudioBuffer::Listener:** `bufferWasLoaded()` and `bufferWasModified()` both trigger `recalculate()`, ensuring the grain collection is regenerated whenever the audio file changes.

## CPU Assessment

- **Offline analysis:** high (FFT-based, but one-time and asynchronous on sample loading thread)
- **Per-voice grain rendering:** low-medium (4 concurrent grains x per-sample linear interpolation + RNG)
- **Modulation + fader:** negligible (per-block)
- **Overall baseline:** low (per voice, runtime only)
- **Scaling factor:** Polyphony count multiplies per-voice cost linearly. Grain size does not affect runtime cost.

## UI Components

Uses `NoiseGrainPlayerEditor` - standard parameter editor with `MultiChannelAudioBufferDisplay` thumbnail, `HiSlider` for Position/Mix/WhiteNoise, and `HiComboBox` for GrainSize. Backend-only (`#if USE_BACKEND`). No FloatingTile content type.

## Notes

- The header file class comment says "A simple stereo panner which can be modulated using all types of Modulators" which is copy-pasted from another class and does not describe this module
- The noiseAmp field is captured at voice start from the WhiteNoise parameter (`voiceState[voiceIndex].noiseAmp = whiteNoise`) - changes to WhiteNoise after a voice has started do not affect that voice's noise amount
- Constructor lines 157-158 call `setIncludeMonophonicValuesInVoiceRendering(true)` twice on `PositionBipolar` (index 1) and never on `PositionChain` (index 0) - appears to be a copy-paste oversight
- The `EditorStates` enum has `PositionBipolarShown` set to the same value as `PositionChainShown` (`Processor::numEditorStates`) instead of incrementing - cosmetic issue in the enum definition
