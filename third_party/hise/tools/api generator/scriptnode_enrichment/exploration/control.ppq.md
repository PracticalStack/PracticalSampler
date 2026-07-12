# control.ppq - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:673`
**Base class:** `transport_base<double, NV>`, `polyphonic_base`
**Classification:** control_source

## Signal Path

Transport start/resync event -> onTransportChange()/onResync() captures ppqPosition -> updateValue() computes normalised position -> handleModulation() (from transport_base) detects change and outputs via wrap::mod.

ppq inherits from `transport_base<double, NV>` which provides the handleModulation() pattern. The node captures the PPQ position on transport start and resync events, wraps it into a loop defined by Tempo and Multiplier, and normalises to 0..1.

## Gap Answers

### output-formula: Confirm the output formula.

Confirmed from `updateValue()`:
```
this->value = hmath::fmod(ppqPos, loopLengthQuarters) / loopLengthQuarters
```
Where `loopLengthQuarters = TempoSyncer::getTempoFactor(t) * factor`. Output is always 0..1 (fmod ensures wrapping, division normalises). If `loopLengthQuarters == 0`, it is clamped to 1.0 to prevent division by zero.

### update-timing: Does ppq only update on transport start and resync?

Confirmed. The node only sets `ppqPos` in two callbacks:
- `onTransportChange(bool isPlaying, double ppqPosition)` -- only when `isPlaying == true`
- `onResync(double newPos)` -- on position jumps (loop boundaries, user seeks)

There is no continuous tracking. The value is NOT updated during ongoing playback. The node captures a snapshot of the PPQ position at event time and wraps it into the loop range. To get continuous PPQ tracking, use a different node (e.g., clock_ramp).

### tempo-default-value: Confirm default Tempo mapping.

The default is set via `p.setDefaultValue((double)TempoSyncer::getTempoIndex("1/4"))`. The `getTempoIndex("1/4")` returns the enum index for Quarter note. Looking at the TempoSyncer enum, Quarter is at index 5 (after Whole=0, HalfDuet=1, Half=2, HalfTriplet=3, QuarterDuet=4, Quarter=5). Default Tempo = 5 = Quarter = "1/4". Confirmed.

## Parameters

- **Tempo**: TempoSyncer enum index (0-18). Selects the musical time value. Stored as `TempoSyncer::Tempo t`. Default "1/4" (index 5).
- **Multiplier**: Integer multiplier (1-16, clamped to 1-64 in code). Multiplies the loop length. Default 1.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

ppq uses `transport_base::handleModulation()` which compares `polyValue.get()` against `value`. If different, it returns true with the new value and updates polyValue. This means the output only fires when the value actually changes. The Multiplier is clamped to `jlimit(1.0, 64.0, v)` in the code, which is wider than the parameter range (1-16).
