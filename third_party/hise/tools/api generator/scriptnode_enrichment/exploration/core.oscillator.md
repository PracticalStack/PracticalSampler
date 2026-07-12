# core.oscillator - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:1400`
**Base class:** `OscillatorDisplayProvider`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

The oscillator generates a waveform and ADDS it to the existing audio signal. Each sample is computed via `processFrameInternal()` which calls the appropriate tick function based on the current Mode, multiplies by gain and Nyquist attenuation, then does `s += v` for every channel in the frame. This is additive -- it does not replace the input.

Processing is frame-based internally. `process()` converts to frame iteration via `toFrameData()` for 2 channels, or manual span reinterpretation for 1 channel.

The oscillator uses a shared `SineLookupTable<2048>` for sine generation. The Saw, Triangle, and Square waveforms are implemented in `Oscillators.h`/`Oscillators.cpp` via tick functions that operate on `OscData` (phase accumulator). Noise mode uses `Random::getSystemRandom()`.

Nyquist attenuation is applied via `OscData::getNyquistAttenuationGain()` to reduce aliasing at high frequencies.

## Gap Answers

### waveform-modes: What waveform does each mode value produce?

The Mode enum in `OscillatorDisplayProvider` (Oscillators.h:118) defines:
- 0 = Sine (lookup table, 2048 entries)
- 1 = Saw
- 2 = Triangle
- 3 = Square
- 4 = Noise (white noise, uniform random)

The `modes` StringArray is {"Sine", "Saw", "Triangle", "Square", "Noise"} (Oscillators.h:167). The `processFrameInternal()` switch at CoreNodes.h:1472 maps: Sine->tickSine, Triangle->tickTriangle, Saw->tickSaw, Square->tickSquare, Noise->random. The `setMode()` method casts the double parameter directly to the `Mode` enum.

### processing-order: Replace or add to output?

Additive. `processFrameInternal()` at line 1481: `for (auto& s : data) s += v;`. The oscillator output is added to whatever signal is already in the buffer. This means multiple oscillators can be stacked in a chain.

### gate-behaviour: Is Gate automatic from MIDI?

Gate is a purely manual parameter -- `handleHiseEvent()` (line 1498) only calls `setFrequency(e.getFrequency())` on note-on. It does NOT set Gate. Gate must be controlled explicitly (default 1.0 = on). When Gate is off (`setGate()` with v <= 0.5), `enabled` is set to 0 and `process()` returns early at line 1443.

### antialiasing: Anti-aliasing method?

The oscillator uses `OscData::getNyquistAttenuationGain()` (Oscillators.h:96) which provides frequency-dependent gain reduction near Nyquist. The waveforms themselves (Saw, Triangle, Square) are naive (no PolyBLEP or band-limiting). The Nyquist attenuation only reduces amplitude at high frequencies -- it does not remove harmonics. The Sine waveform uses a 2048-point lookup table with interpolation, so it is inherently band-limited.

### display-buffer-usage: What does the display show?

The oscillator inherits from `OscillatorDisplayProvider` which inherits from `display_buffer_base<true>`. The `OscillatorDisplayObject` renders the current waveform shape (one cycle) using `uiData` (which stores the UI-facing oscillator state). The display shows the theoretical waveform shape, not the real-time output.

### freq-ratio-interaction: How do Frequency and Freq Ratio interact?

The effective frequency is `Frequency * FreqRatio`. In `setFrequency()` (line 1557), `uptimeDelta = frequency / sr * tableSize`. In `processFrameInternal()`, `currentVoiceData->tick()` advances uptime by `uptimeDelta * multiplier`. The `multiplier` field is set by `setPitchMultiplier()`. So the actual phase increment per sample is `(frequency / sampleRate) * tableSize * pitchMultiplier`.

## Parameters

- **Mode** (0-4): Selects waveform. Sine/Saw/Triangle/Square/Noise.
- **Frequency** (20-20000 Hz): Base frequency. Overridden by MIDI note-on.
- **Freq Ratio** (1-16, integer): Multiplier applied to frequency. Stored in OscData.multiplier.
- **Gate** (0/1): Enables/disables output. When transitioning from off to on, phase resets to 0.
- **Phase** (0-1): Phase offset in table units. Stored in OscData.phase.
- **Gain** (0-1): Output amplitude multiplier applied before Nyquist attenuation.

## Polyphonic Behaviour

`PolyData<OscData, NumVoices> voiceData` stores per-voice oscillator state (uptime, uptimeDelta, multiplier, phase, gain, enabled). Each voice runs independently. MIDI note-on sets frequency for the current voice via `handleHiseEvent()`.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: [{"parameter": "Mode", "impact": "negligible", "note": "Noise mode avoids table lookup but uses random"}]
