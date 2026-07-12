# core.clock_ramp - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:862`
**Base class:** `polyphonic_base`, `data::display_buffer_base<UseRingBuffer>`, `hise::TempoListener`
**Classification:** audio_processor (also modulation source)

## Signal Path

The clock_ramp generates a 0..1 ramp synchronized to the HISE/DAW clock. It outputs via modulation and optionally adds to the audio signal when AddToSignal is enabled.

In `process()` (line 989): `ptr[i] += s.tick() * addToSignalGain` on channel 0 only. When AddToSignal=0, `addToSignalGain` is 0.0f so no audio modification occurs.

The ramp value is always output as a modulation signal via `handleModulation()` (line 975) which returns `clockState.get().getModValue()` unconditionally (always returns true).

The node registers as a `TempoListener` in `prepare()` and receives `tempoChanged()`, `onTransportChange()`, and `onResync()` callbacks.

## Gap Answers

### update-mode-difference: Continuous vs Synced?

Two modes controlled by `setUpdateMode()` (line 1037). In the `State::tick()` method (line 1120):
- **Continuous** (UpdateMode < 0.5): `fmod((uptime + offset*factor) * deltaPerSample * factor, 1.0)` -- the offset is scaled by the tempo factor before adding to uptime. This means phase is calculated from the absolute PPQ position.
- **Synced** (UpdateMode >= 0.5): `fmod((offset + uptime*deltaPerSample) * factor, 1.0)` -- offset and uptime are added in PPQ space, then scaled. The practical difference: Continuous gives a ramp that tracks the absolute DAW position, while Synced gives a ramp that restarts on transport changes and note-on events.

### inactive-modes: What does each Inactive value output?

Confirmed in State constructor (line 1108) and `getModValue()` (line 1116):
- 0 = LastValue: outputs the last computed ramp value (frozen when transport stops)
- 1 = Zero: outputs 0.0 when transport is not playing
- 2 = One: outputs 1.0 when transport is not playing

The `getModValue()` uses `inactive[inactiveIndex * (1 - (int)isPlaying)]` -- when playing, index is 0 so it returns LastValue (the live ramp); when stopped, it returns the selected inactive value.

### add-to-signal-behaviour: AddToSignal behaviour?

When AddToSignal=1, `addToSignalGain` is 1.0f and the ramp value is added to channel 0 of the audio signal. When AddToSignal=0, `addToSignalGain` is 0.0f and the audio passes through unmodified. In both cases, the modulation output is always active.

### tempo-sync-mechanism: How does tempo sync work?

The node registers as a TempoListener via `syncer->registerItem(this)` in `prepare()`. It receives:
- `tempoChanged(bpm)`: stores bpm, triggers recalculation of deltaPerSample
- `onTransportChange(isPlaying, ppqPosition)`: sets play state, resets uptime, captures ppq offset
- `onResync(ppqPosition)`: resets offset and uptime for loop boundary handling

The `deltaPerSample` is computed as `1.0 / quarterInSamples` (one quarter note maps to one unit). The `factor` is `1.0 / (tempoFactor * multiplier)` where tempoFactor comes from the selected Tempo enum. This makes the ramp complete one cycle per the selected note duration.

### hise-event-usage: What MIDI events are handled?

`handleHiseEvent()` (line 925): responds to note-on events only (when polyphonic). On note-on, it captures the PPQ position at the event's timestamp and resets uptime to 0. This resyncs the ramp to the note start position.

## Parameters

- **Tempo** (0-18, default 5="1/4"): Selects the musical note value from TempoSyncer enum.
- **Multiplier** (1-16, integer, default 1): Additional period multiplier.
- **AddToSignal** (No/Yes, default No): Whether to add ramp to audio channel 0.
- **UpdateMode** (Continuous/Synced, default Synced): Phase calculation method.
- **Inactive** (Current/Zero/One, default Current): Output when transport is stopped.

## Polyphonic Behaviour

`PolyData<State, NV> clockState` stores per-voice ramp state including uptime, offset, delta, tempo settings, and inactive values.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
