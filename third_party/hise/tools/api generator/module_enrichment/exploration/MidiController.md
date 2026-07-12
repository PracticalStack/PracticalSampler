# Midi Controller - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/ControlModulator.h`, `hi_core/hi_modules/modulators/mods/ControlModulator.cpp`
**Base class:** `TimeVariantModulator` (also implements `LookupTableProcessor`, `MidiControllerAutomationHandler::MPEData::Listener`)

## Signal Path

MIDI event (CC / aftertouch / pitch wheel) -> normalize to 0-1 -> constrain to 0-1 -> [optional] table lookup -> [optional] invert -> set targetValue -> smoother (one-pole low-pass) -> output buffer

The modulator listens for HiseEvents in `handleHiseEvent()`. When the event matches the configured `controllerNumber`, the raw value is normalized to 0-1. If `useTable` is enabled, the normalized value is passed through the lookup table. If `inverted` is enabled, the result is flipped (1 - value). The result becomes `targetValue`. In `calculateBlock()`, the smoother interpolates `currentValue` toward `targetValue` sample-by-sample using a one-pole IIR filter, writing the smoothed values into `internalBuffer`. The base class `TimeVariantModulator::render()` then calls `calculateBlock()` and applies the result via `applyTimeModulation()`.

## Gap Answers

### signal-path-order: What is the exact processing order?

The order is: **normalize -> constrain -> table -> invert -> smooth -> output**.

In `handleHiseEvent()`:
1. Raw MIDI value is normalized: CC divides by 127.0, pitch wheel by 16383.0, aftertouch by 127.0
2. `CONSTRAIN_TO_0_1(inputValue)` clamps the result
3. If `useTable`: `value = table->getInterpolatedValue((double)inputValue, sendNotificationAsync)` -- table input domain is 0-1 (normalized CC), table output is 0-1
4. If `inverted`: `value = 1.0f - value` -- inversion happens AFTER table lookup
5. `targetValue = value`

In `calculateBlock()`:
6. The smoother interpolates `currentValue` toward `targetValue` per-sample using `smoother.smooth(targetValue)`

Note: table lookup receives the **pre-inversion** normalized value. Inversion is applied to the table output.

### special-controller-modes: How do ControllerNumber 128 (aftertouch) and 129 (pitch wheel) work?

**Important correction:** The preliminary JSON has the assignments swapped. In the code:
- `HiseEvent::PitchWheelCCNumber = 128` (pitch wheel)
- `HiseEvent::AfterTouchCCNumber = 129` (aftertouch)

**Pitch wheel (controllerNumber == 128):** Checks `m.isPitchWheel()`. Normalizes 14-bit value via `m.getPitchWheelValue() / 16383.0f`, giving range 0-1 where 0.5 is center. This is the same normalization as the dedicated PitchWheel modulator.

**Aftertouch (controllerNumber == 129):** Accepts both channel pressure (`m.isChannelPressure()`) and polyphonic aftertouch (`m.isAftertouch()`). Channel pressure is normalized by dividing by 127. Polyphonic aftertouch is more complex: each note's aftertouch is stored in a 128-element `polyValues[]` array (indexed by note number), and the final `inputValue` is the **maximum** across all active notes. Notes are cleared from `polyValues` on note-off. This means the modulator always reflects the highest aftertouch pressure across all held notes.

### cc-normalization: How are 7-bit CC values (0-127) normalized to 0-1?

Simple division: `inputValue = (float)m.getControllerValue() / 127.0f`. CC 0 maps to 0.0, CC 64 maps to 0.503937 (not exactly 0.5), CC 127 maps to 1.0.

### table-lookup-position: Where does the TableProcessor lookup occur in the signal path?

The table receives the **normalized, pre-inversion** value (0-1). `table->getInterpolatedValue((double)inputValue, ...)` where `inputValue` is the 0-1 normalized CC value after `CONSTRAIN_TO_0_1`. The method internally scales 0-1 to the 512-entry table index. The table X axis represents normalized CC value (0-127 displayed via `getDomainAsMidiRange` converter which multiplies by 127). The table output replaces the CC value before inversion is applied.

### default-value-behavior: Is DefaultValue used only at initialization, or does it reset?

When `DefaultValue` is set via `setInternalAttribute`, it calls `handleHiseEvent(HiseEvent(HiseEvent::Type::Controller, (uint8)controllerNumber, (uint8)defaultValue, 1))`. This synthesizes a fake CC event and runs it through the full signal path (normalize, table, invert, smooth).

**However, there is a bug:** `defaultValue` is stored as a float in 0.0-1.0 range (NormalizedPercentage), but the synthesized HiseEvent casts it to `uint8`: `(uint8)defaultValue`. Since `defaultValue` is 0.0-1.0, the cast always produces 0 or 1. This value then gets divided by 127.0 in `handleHiseEvent`, resulting in effectively 0.0 or ~0.008 as the initial modulation value. The DefaultValue parameter is functionally broken for any value other than 0.

The DefaultValue is applied when the parameter is set (including on preset load via `restoreFromValueTree` -> `loadAttribute` -> `setInternalAttribute`). It does not reset on all-notes-off.

### smoothing-implementation: What smoothing algorithm is used?

The `Smoother` class (aliased from `DownsampledSmoother<1>`) implements a **one-pole IIR low-pass filter**. The coefficient calculation in `setSmoothingTime()`:
- `freq = 1000.0 / smoothTime` (converts ms to Hz)
- `x = exp(-2 * pi * freq / sampleRate)`
- `a0 = 1 - x`, `b0 = -x`
- `smooth(newValue) = a0 * newValue - b0 * prevValue`

This is the standard exponential smoothing formula (one-pole low-pass). The time constant represents the -3dB point of the filter. The smoother runs at the control rate (same as audio sample rate since `DownsamplingFactor=1`). This is the same `Smoother` class used by PitchWheel and other modulators -- the implementation is identical.

When `smoothTime == 0`, the smoother is deactivated (`active = false`) and `smooth()` returns the input value unmodified.

## Processing Chain Detail

1. **MIDI Event Filtering** (in `handleHiseEvent`): Checks event type against `controllerNumber`. MPE guard skips non-channel-1 events when MPE is enabled. MIDI Learn mode intercepts any CC/aftertouch/pitchwheel. No CPU cost when no matching events arrive. -- *negligible*

2. **Normalization** (in `handleHiseEvent`): Divides raw MIDI value by 127 (CC, aftertouch) or 16383 (pitch wheel). For polyphonic aftertouch, updates per-note array and finds maximum. -- *negligible*

3. **Table Lookup** (in `handleHiseEvent`, conditional on `useTable`): 512-point linear interpolation lookup. Only executes on MIDI event, not per-sample. -- *negligible*

4. **Inversion** (in `handleHiseEvent`, conditional on `inverted`): `1.0 - value`. -- *negligible*

5. **Smoothing** (in `calculateBlock`): Per-sample one-pole IIR filter when target differs from current. Falls back to `FloatVectorOperations::fill` when converged. -- *low*

## Modulation Points

No modulation chains. The module has no child processors (`getNumChildProcessors` returns 0). All parameters are static (set by user or scripting, not modulated).

## Conditional Behavior

- **UseTable (0/1):** When enabled, normalized inputValue is passed through the SampleLookupTable before inversion. When disabled, normalized value is used directly.
- **Inverted (0/1):** When enabled, `value = 1.0 - value` is applied after table lookup (or after normalization if table is off).
- **ControllerNumber:** Determines which MIDI event type is matched:
  - 0-127: Standard CC number
  - 128: Pitch wheel (14-bit, normalized by 16383)
  - 129: Aftertouch (both channel pressure and polyphonic, with max-of-active-notes aggregation)
- **MPE mode:** When MPE is globally enabled, only channel 1 events are processed (non-channel-1 events are filtered out to avoid interference with MPE per-note expression).
- **SmoothTime:** When 0, smoother is bypassed (direct pass-through). Otherwise, one-pole IIR filter is active.
- **Learn mode:** When active, the next CC/aftertouch/pitchwheel event sets `controllerNumber` and exits learn mode.

## Interface Usage

**TableProcessor (LookupTableProcessor):** One table (index 0), a `SampleLookupTable` with 512 entries. The X-axis text converter is set to `getDomainAsMidiRange` (displays 0-127). The table is queried in `handleHiseEvent` with the normalized input value (0-1), which `getInterpolatedValue` internally scales to the 512-entry range. The table output (0-1) replaces the input value before inversion. The table is only active when `useTable == true`.

## Vestigial / Notable

The `calculateBlock` method has dead code at lines 237-240: it checks `useTable && lastInputValue != inputValue` and updates `lastInputValue`, but does nothing else with this information. This appears to be a leftover from when the table display was updated from `calculateBlock` rather than from `handleHiseEvent`.

The DefaultValue parameter is effectively broken due to the `(uint8)defaultValue` cast on a 0-1 float value. See issues.md for details.

## CPU Assessment

- **Baseline:** negligible to low. The module only does per-sample work in `calculateBlock` (the smoother), and even that short-circuits to a `fill` operation when converged.
- **No parameters scale cost.** Table lookup is per-event, not per-sample.
- **Overall tier: low**

## UI Components

The editor class is `ControlEditorBody` (defined in `modulators/editors/ControlEditor.h`). It contains:
- `controllerNumberSlider` (HiSlider, Discrete mode, range 0-129)
- `smoothingSlider` (HiSlider, Time mode)
- `defaultSlider` (HiSlider, NormalizedPercentage)
- `useTableButton` (HiToggleButton)
- `invertedButton` (HiToggleButton)
- `learnButton` (ToggleButton for MIDI Learn)
- `midiTable` (TableEditor, shown/hidden based on UseTable)

No FloatingTile content types are registered; this is a standard ProcessorEditorBody.

## Notes

- The module is monophonic (shared state, one `targetValue`/`currentValue` pair). It processes all MIDI events on the audio thread via `handleHiseEvent`.
- The polyphonic aftertouch aggregation (max across all held notes) is a notable design choice. It means the modulator always reflects the highest finger pressure, not per-voice pressure.
- The `polyValues` array is cleared note-by-note on noteOff (`polyValues[noteNumber] = -1.0f`), with negative values excluded from the maximum by the `if (inputValue < 0.0f) inputValue = 0.0f` guard.
- The old Jucer-based editor had the CC number slider range as 1-128 (1-based display), but the current metadata uses 0-129 (0-based). The forum insight about 1-based display in UI refers to the legacy editor; the current system is 0-based.
- Constructor initializes `targetValue` and `currentValue` to 1.0f, not 0.0f. This means the modulator starts at full output (1.0) before any CC is received, regardless of DefaultValue (which is applied during `restoreFromValueTree`).
