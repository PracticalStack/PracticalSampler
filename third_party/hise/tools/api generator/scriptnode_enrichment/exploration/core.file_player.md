# core.file_player - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:1647`
**Base class:** `data::base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Plays audio from an AudioFile slot with three distinct playback modes. Output is additive in Static/MidiFreq modes (`data += fd` in `processWithPitchRatio`), but replaces in SignalInput mode (`data[i] = fd[i]` in `processWithSignalInput`).

Uses `DataTryReadLock` for thread-safe access to the audio file data. If the lock fails, the buffer is skipped (or cleared in SignalInput mode).

Processing is frame-based internally. `process()` dispatches to `processFix<C>()` which uses `toFrameData()` iteration.

## Gap Answers

### playback-modes: What are the three modes?

Enum `PlaybackModes` (line 1656):
- **Static** (0): Plays at a fixed pitch determined by the audio file's metadata. `Gate` triggers playback. Uses `processWithPitchRatio()` with `uptimeDelta` from the StereoSample pitch factor. Loops using the audio file's loop range.
- **SignalInput** (1): Uses the input signal on channel 0 as a normalised position (0..1) into the audio file. Like a wavetable lookup -- the input drives the read position. Replaces the audio signal with the file content at that position.
- **MidiFreq** (2): Pitch-tracks MIDI note-on events. On note-on, computes `uptimeDelta` from `noteFrequency / rootFreq` or from the StereoSample's pitch factor. Additive output. Supports XYZ sample mapping (velocity/note layers).

### gate-behaviour: Gate interaction with MIDI?

`setGate()` (line 1952): when Gate goes on, resets all voice uptimes to 0 and sets uptimeDelta to 1.0. When Gate goes off, sets uptimeDelta to 0 (stops playback). In MidiFreq mode, note-on events in `handleHiseEvent()` (line 1905) override the uptimeDelta with the pitch ratio and reset uptime. Gate and MIDI note-on are independent triggers.

### freq-ratio-pitch-tracking: How do RootFrequency and FreqRatio interact?

In MidiFreq mode: `uptimeDelta = noteFrequency / rootFreq` (line 1921). Then in `processWithPitchRatio()`, the actual playback speed is `OscData.tick()` which uses `uptimeDelta * multiplier` where `multiplier` is set by `setFreqRatio()`. So effective speed = `(noteFreq / rootFreq) * freqRatio * globalRatio` where `globalRatio = fileSampleRate / nodeSampleRate`.

### audio-file-usage: Stereo, interpolation, end-of-file?

Supports stereo via `StereoSample` which holds two channel blocks. Uses `index::lerp` with `index::looped<0>` for interpolation with loop support. When playback reaches the end of the loop range, it wraps. Linear interpolation between samples.

### process-order: Replace or add?

Mode-dependent. Static/MidiFreq: additive (`data += fd`). SignalInput: replaces (`data[i] = fd[i]`). In SignalInput mode with empty data, output is cleared to silence.

## Parameters

- **PlaybackMode** (Static/Signal in/MIDI, default Static): Selects playback behaviour.
- **Gate** (Off/On, default On): Starts/stops playback. On resets position.
- **RootFrequency** (20-2000 Hz, default 440): Reference frequency for pitch calculation.
- **FreqRatio** (0-2, default 1): Playback speed multiplier.

## Conditional Behaviour

The three PlaybackModes produce fundamentally different behaviour:
- Static: one-shot/looped file playback at original pitch
- SignalInput: audio-rate position scrubbing (wavetable-like)
- MidiFreq: polyphonic pitch-tracked sample playback

## Polyphonic Behaviour

`PolyData<OscData, NumVoices> state` and `PolyData<StereoSample, NUM_POLYPHONIC_VOICES> currentXYZSample` store per-voice playback position and sample selection.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: [{"parameter": "PlaybackMode", "impact": "low", "note": "SignalInput mode uses different interpolation path"}]
