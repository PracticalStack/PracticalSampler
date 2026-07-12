# core.stretch_player - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/StretchNode.h:10`
**Base class:** `data::base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Plays back an audio file with independent time stretching and pitch shifting. The node is fixed to 2 channels (stereo). The signal path depends on the `enabled` flag:

**When enabled (timestretching on):** Source audio samples are read from the current position, fed through a `time_stretcher` object (Signalsmith Stretch library wrapper), and output at the target block size. The stretcher handles the time-domain manipulation. If the source sample rate differs from the processing sample rate, resampling is applied by adjusting the number of input samples consumed per block.

**When disabled (timestretching off):** Samples are read directly from the audio file buffer using either hermite interpolation (when playback ratio != 1.0) or direct integer indexing (when ratio == 1.0). This provides a simple file player mode without timestretching overhead.

Loop wrapping is handled explicitly: when the read position exceeds the source length, samples are copied into a loop buffer that spans the wrap point, then fed to the stretcher.

## Gap Answers

### timestretching-algorithm

The `time_stretcher` class (`hi_streaming/timestretch/time_stretcher.h`) is a pimpl wrapper around the **Signalsmith Stretch** library (a phase-vocoder-based algorithm). The comment in the header says "A pimpl wrapper around the signalsmith stretcher." The `time_stretcher` delegates to a `timestretch_engine_base` which provides `process()`, `configure()`, `setTransposeSemitones()`, `setTransposeFactor()`, and `setFFTSize()` methods. Engine implementations are registered via factory functions, allowing alternative engines (e.g., RubberBand if `HISE_ENABLE_RUBBERBAND` is defined).

### timeratio-behaviour

TimeRatio is a **speed multiplier** on the time-stretching ratio. A value of 1.0 means original speed, values > 1.0 mean faster playback (consuming more source samples per output block), and values < 1.0 mean slower playback. In `processFix()`, the number of input samples consumed is calculated as `numSamplesToProduce * ratio`, where `ratio` comes from `syncer.getRatio(s.timeRatio)`. When ClockSync is active, the ratio is overridden by `bpm / sourceBpm`. The parameter is clamped to 0.5-2.0 in `setParameter<1>`.

### pitch-units

Pitch is in **semitones** (-12 to +12, i.e., one octave each direction). It is independent of time stretching. In `setParameter<2>`, the value is passed directly to `s.stretcher.setTransposeSemitones(s.pitchRatio, 0.17)` where 0.17 is the tonality parameter. The actual clamping in the code is -24 to +24 (wider than the parameter range), allowing modulation to push beyond the UI range.

### enable-parameter

When Enable is set to 0 (`enabled = false`), the node **bypasses timestretching and plays the audio file directly** (like a simple file player). The `processFix()` method checks `if(enabled)` and takes the non-stretching path: samples are read using hermite interpolation (or direct indexing at ratio 1.0) with position advancing by `playbackRatio` per sample. The stretcher is not invoked. Playback continues -- it does not stop.

### clocksync-behaviour

When ClockSync is enabled, the `tempo_syncer` object:
1. Registers as a `TempoListener` to receive BPM changes from the host.
2. Calculates `sourceBpm` from the audio file's duration and an estimated quarter-note count (auto-detected by rounding to the nearest power-of-2 quarter count).
3. Overrides the TimeRatio with `bpm / sourceBpm` (clamped to max 2.0) via `syncer.getRatio()`.
4. Syncs playback position to the DAW transport: `onTransportChange()` and `onResync()` update a `resyncPosition` ModValue that resets the read position to match the DAW's PPQ position.
5. Transport start/stop is forwarded to the gate state via `syncer.updatePlayback()`.

### gate-midi-interaction

Gate is parameter index 0. When Gate transitions to true (> 0.5), it calls `seek(0.0)` which resets playback to the beginning (with stretcher latency skip). The node also inherits from `polyphonic_base` which registers `IsProcessingHiseEvent`, but `SN_EMPTY_HANDLE_EVENT` is declared -- so MIDI note events do NOT directly trigger playback. The Gate parameter must be driven explicitly (e.g., by a control node or automation). Per-voice state exists (`PolyData<State, NV>`) so in polyphonic context each voice has independent playback position, gate, and stretcher state.

## Parameters

- **Gate** (0/1, default 1): Starts/stops playback. Transition to 1 seeks to position 0. Not driven by MIDI directly.
- **TimeRatio** (0.5-2.0, skew centre 1.0, default 1.0): Playback speed multiplier. 1.0 = original speed. Overridden by ClockSync when active.
- **Pitch** (-12 to +12, default 0): Pitch shift in semitones, independent of time. Internally clamped to -24..+24.
- **Enable** (0/1, default 1): Toggles timestretching. Off = direct playback with resampling interpolation.
- **ClockSync** (0/1, default 0): Syncs playback speed and position to DAW tempo/transport.

## Conditional Behaviour

1. **Enable on/off**: Controls whether the stretcher is used or direct sample reading occurs.
2. **ClockSync on/off**: When on, overrides TimeRatio with tempo-derived ratio and syncs position to DAW transport.
3. **playbackRatio == 1.0 (in non-stretch mode)**: Uses direct integer indexing instead of hermite interpolation.

## Polyphonic Behaviour

Template parameter NV controls voice count. `PolyData<State, NV> state` stores per-voice: `pitchRatio`, `timeRatio`, `currentPosition`, `leftOver`, `stretcher` (full time_stretcher instance), and `gate`. The `tempo_syncer` also has `PolyData<State, NV>` for per-voice resync positions. Each voice has its own independent stretcher instance -- this can be memory-intensive with high voice counts.

## CPU Assessment

baseline: high
polyphonic: true
scalingFactors:
  - parameter: Enable, impact: high, note: "Timestretching (FFT-based) is significantly more expensive than direct playback"
  - parameter: NumVoices, impact: high, note: "Each voice has its own time_stretcher instance"

The Signalsmith Stretch algorithm uses FFT-based processing which is computationally expensive. Each polyphonic voice instantiates a full stretcher. When timestretching is disabled, CPU drops to low (simple buffer reads with optional interpolation).

## Notes

The node is fixed to 2 channels (`getFixChannelAmount() = 2`). The `process()` template method checks `data.getNumChannels() != 2` and returns early if not stereo. Audio file sources are limited -- `setExternalData()` disables SampleMap and SFZ providers, allowing only AudioFile sources. The `hasTail()` returns true and `isSuspendedOnSilence()` returns false, meaning the node continues processing even without input. Display position is updated via `ed.setDisplayedValue()`.
