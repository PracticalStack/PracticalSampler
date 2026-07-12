# Pitch Wheel Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/PitchWheelModulator.h`, `hi_core/hi_modules/modulators/mods/PitchWheelModulator.cpp`
**Base class:** `TimeVariantModulator` (via `Modulator` + `TimeModulation`)

## Signal Path

MIDI pitch bend event -> normalize to 0-1 (divide by 16383) -> table lookup (optional) -> invert (optional) -> set as smoothing target -> per-sample exponential smoothing -> output to internalBuffer

The processing is split across two methods:

1. `handleHiseEvent()` runs on the MIDI thread when a pitch bend message arrives. It normalizes the raw 14-bit value, optionally passes it through the table, optionally inverts, and stores the result as `targetValue`.
2. `calculateBlock()` runs at the control rate and smooths `currentValue` toward `targetValue` using a one-pole lowpass filter, writing per-sample values into `internalBuffer`.

The base class `TimeVariantModulator::render()` calls `calculateBlock()` then `applyTimeModulation()` which multiplies the internalBuffer values into the modulation output buffer (applying intensity).

## Gap Answers

### pitchbend-to-normalized-mapping: How is the 14-bit MIDI pitch bend value (0-16383) mapped to the 0-1 modulation range? Is center position (8192) mapped to 0.5?

Yes. In `handleHiseEvent()`, the raw pitch wheel value is divided by 16383.0f:

```
inputValue = m.getPitchWheelValue() / 16383.0f;
```

This maps 0 -> 0.0, 8192 -> 0.50003 (approximately 0.5), 16383 -> 1.0. Center position maps to ~0.5.

The constructor initializes both `targetValue` and `currentValue` to 0.5f, confirming that the default (no pitch bend received) corresponds to center position.

### signal-path-order: What is the exact processing order?

The order in `handleHiseEvent()` is:

1. Normalize: `inputValue = getPitchWheelValue() / 16383.0f`
2. Table lookup (if `useTable`): `value = getTableUnchecked()->getInterpolatedValue(inputValue, ...)`
3. Invert (if `inverted`): `value = 1.0f - value`
4. Store as target: `targetValue = value`

Then in `calculateBlock()`:

5. Per-sample smoothing toward targetValue using exponential lowpass
6. Write smoothed values to `internalBuffer`

So the order is: normalize -> table -> invert -> smooth -> output. Note that inversion happens AFTER table lookup.

### table-lookup-position: Where does the TableProcessor lookup occur in the signal path?

The table receives the normalized (0-1) value BEFORE inversion. The table input domain represents the raw normalized pitch wheel position: 0.0 = full down, 0.5 = center, 1.0 = full up. The table's X-axis text converter is set to `getDomainAsPitchBendRange` which displays the input as -8192 to +8192 using `jmap<float>(input, -8192.0f, 8192.0f)`.

If the Inverted parameter is enabled, inversion (1 - value) is applied to the TABLE OUTPUT, not the table input. This means the table always sees the same input regardless of the Inverted setting.

### smoothing-implementation: What smoothing algorithm is used?

The `Smoother` class (alias for `DownsampledSmoother<1>`) implements a one-pole exponential lowpass filter. The `smooth()` method computes:

```
currentValue = a0 * newValue - b0 * prevValue
```

where the coefficients are derived from the smoothing time:

```
freq = 1000.0 / smoothTimeMs
x = exp(-2 * pi * freq / sampleRate)
a0 = 1 - x
b0 = -x
```

This is a standard first-order IIR lowpass. Smoothing is applied **per-sample** in `calculateBlock()`. The smoother runs at the control rate (set via `smoother.prepareToPlay(getControlRate())`), not the full audio sample rate.

When `smoothTime == 0.0`, the smoother deactivates (`active = false`) and `smooth()` returns the input unchanged, saving CPU.

In `calculateBlock()`, there is a fast-path optimization: if the difference between `targetValue` and `currentValue` is below the silence threshold (`FloatSanitizers::isNotSilence` returns false), the block is filled with a constant value using `FloatVectorOperations::fill()` instead of per-sample processing.

### inversion-formula: Does inversion use 1.0 - value, or does it mirror around center (0.5)?

Inversion uses simple `1.0f - value`. Since the normalized pitch wheel range is 0-1 with center at 0.5, this is equivalent to mirroring around 0.5. Full down (0.0) becomes 1.0, center (0.5) stays 0.5, full up (1.0) becomes 0.0.

## Processing Chain Detail

1. **MIDI Event Reception** (`handleHiseEvent`): Filters for pitch wheel events. If MPE is enabled, only channel 1 events are processed (non-MPE master channel events are ignored). Per-event, negligible CPU.

2. **Normalization** (`handleHiseEvent`): Divides 14-bit integer (0-16383) by 16383.0f to produce 0.0-1.0 float. Negligible CPU.

3. **Table Lookup** (`handleHiseEvent`, conditional on `useTable`): Calls `getInterpolatedValue()` on the 512-point lookup table. Runs once per MIDI event, not per sample. Negligible CPU.

4. **Inversion** (`handleHiseEvent`, conditional on `inverted`): `1.0f - value`. Negligible CPU.

5. **Exponential Smoothing** (`calculateBlock`): Per-sample one-pole IIR lowpass toward `targetValue`. Runs at control rate for every audio block. Low CPU. Has fast-path when target is reached (constant fill).

All stages are monophonic (shared state, no per-voice processing).

## Modulation Points

The PitchWheel modulator has no child modulation chains (`getNumChildProcessors` returns 0). It is itself a modulation source. Its output is written to `internalBuffer` and then applied to the parent chain's modulation buffer by the base class `applyTimeModulation()`.

The intensity parameter (inherited from `Modulation` base) scales the output. In a pitch chain, intensity range -1 to +1 maps to semitone range (typically +/-12 semitones at full intensity), as confirmed by forum insights.

## Interface Usage

### TableProcessor (LookupTableProcessor)

One table (index 0) is created in the constructor. The table's X-axis text converter displays values as -8192 to +8192 (pitch bend range). The table receives the normalized 0-1 input value and outputs a 0-1 transformed value. The table lookup happens per MIDI event in `handleHiseEvent()`, not per sample in `calculateBlock()`. This means table changes take effect immediately on the next pitch bend message, not retroactively on the current smoothing target.

## Conditional Behavior

**UseTable (bool):** When enabled, the normalized pitch wheel value passes through the lookup table before inversion. When disabled, the raw normalized value is used directly.

**Inverted (bool):** When enabled, applies `1.0 - value` after table lookup (or after normalization if table is disabled). The inversion point is 0.5 (center), so center position is unaffected.

**MPE mode (`mpeEnabled`):** When MPE is enabled (via the `MPEData::Listener` callback), only pitch bend messages on MIDI channel 1 are processed. Messages on channels 2-16 are ignored (those are handled by per-voice MPE modulators instead).

**SmoothTime = 0:** Deactivates the smoother entirely. The target value passes through unsmoothed, and `calculateBlock()` uses constant-fill for the entire block when stable.

## CPU Assessment

- **Per-event processing (handleHiseEvent):** Negligible. A few float operations per pitch bend message.
- **Per-block processing (calculateBlock):** Low. Per-sample IIR smoothing at control rate, with fast-path constant fill when not actively smoothing.
- **Overall baseline:** Negligible. This module has minimal CPU impact even at high pitch wheel activity.

## UI Components

The editor class is `PitchWheelEditorBody` (from `PitchWheelEditor.h`/`.cpp`). No FloatingTile content types discovered.

## Notes

- The constructor initializes `targetValue = 0.5f` and `currentValue = 0.5f`. Since center pitch bend (8192) maps to ~0.5, this means the modulator starts at center position before any MIDI is received. This explains the forum-reported "40-50% display bar at init" -- it is not a bug but the correct initial state representing center/no-bend. The display showing ~50% rather than 0% is because 0.5 IS the rest position for pitch bend, not 0.0.

- The `calculateNewValue()` private method appears to be vestigial or used only for legacy display purposes. The main signal path uses `calculateBlock()`, not `calculateNewValue()`. Both implement the same smoothing logic.

- The `inputValue` member stores the last raw normalized value (before table/inversion) but is only written, never read outside `handleHiseEvent`. It may be used for display purposes in the editor.
