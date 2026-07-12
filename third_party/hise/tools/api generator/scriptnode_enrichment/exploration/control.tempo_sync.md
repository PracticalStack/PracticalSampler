# control.tempo_sync - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:466`
**Base class:** `tempo_sync_base` (extends `clock_base`), `polyphonic_base`, `pimpl::no_mod_normalisation`
**Classification:** control_source

## Signal Path

BPM change (via TempoListener::tempoChanged) or parameter change -> TempoData::refresh() recalculates milliseconds -> handleModulation() detects change -> wrap::mod forwards raw milliseconds to targets.

tempo_sync converts a musical tempo value to a duration in milliseconds. It inherits from `clock_base` (TempoListener) and receives `tempoChanged()` callbacks. The output is unnormalised (raw ms values).

## Gap Answers

### output-value-units: Confirm output units.

Confirmed. From `TempoData::refresh()`:
- When `enabled == true`: `currentTempoMilliseconds = TempoSyncer::getTempoInMilliSeconds(bpm, currentTempo) * multiplier`
- When `enabled == false`: `currentTempoMilliseconds = unsyncedTime`

Output is always in milliseconds. The `handleModulation()` in TempoData checks `lastMs != currentTempoMilliseconds` and returns the ms value.

### hise-event-usage: Why is IsProcessingHiseEvent set?

Looking at the C++ source, tempo_sync inherits from `polyphonic_base(getStaticId())` which by default sets `IsProcessingHiseEvent` (the `addProcessEventFlag` defaults to true in `polyphonic_base`). However, the node has `SN_EMPTY_HANDLE_EVENT`, meaning it does nothing with MIDI events. The IsProcessingHiseEvent flag is a side effect of polyphonic_base, not an intentional feature. The node does not respond to MIDI events.

### update-frequency: When does the output update?

The output updates when:
1. BPM changes (via `tempoChanged()` callback, which sets `bpm` and calls `refresh()`)
2. Any parameter changes (Tempo, Multiplier, Enabled, UnsyncedTime all call `refresh()`)

The `handleModulation()` method in TempoData only returns true when `lastMs != currentTempoMilliseconds`, preventing redundant updates. On voice start, the existing value is available but only fires if it has changed since last check.

## Parameters

- **Tempo**: TempoSyncer enum index (0-18). Selects musical time value. Stored as `TempoSyncer::Tempo`.
- **Multiplier**: Integer multiplier (1-16, clamped to 1-32 internally). Multiplies the tempo duration. Default 1.
- **Enabled**: On/Off toggle. When off, outputs UnsyncedTime instead of tempo-synced value. Default Off.
- **UnsyncedTime**: Manual time in milliseconds (0-1000, step 0.1). Used when Enabled=false. Default 200ms.

## Polyphonic Behaviour

Uses `PolyData<TempoData, NV>` for per-voice state. The `handleModulation()` checks `data.isMonophonicOrInsideVoiceRendering()` before accessing per-voice data, preventing updates outside of voice rendering context.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

All audio processing callbacks are empty (SN_EMPTY_PROCESS, etc). The node is purely event-driven via TempoListener callbacks and parameter changes. Despite having empty process methods, it uses wrap::mod for the modulation output. The Multiplier is clamped to `jlimit(1.0, 32.0, v)` in code, wider than the parameter range.
