# Velocity Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/VelocityModulator.h`, `hi_core/hi_modules/modulators/mods/VelocityModulator.cpp`
**Base class:** `VoiceStartModulator`, `LookupTableProcessor`

## Signal Path

The signal path is a linear chain with no branches or feedback:

noteOn velocity (0-1) -> [optional invert] -> [optional table lookup] -> [optional dB conversion] -> modulation output (0-1)

All processing happens in `calculateVoiceStartValue()`, which is called once per voice on note-on. The three optional stages are controlled by boolean parameters and execute in fixed order: invert first, table second, decibel conversion last.

## Gap Answers

### signal-path-order: What is the exact processing order?

The order is strictly sequential in `calculateVoiceStartValue()`:

1. Read float velocity from the MIDI event (0.0 - 1.0 range)
2. If `Inverted` is on: `value = 1.0 - value`
3. If `UseTable` is on: `value = table.getInterpolatedValue(value)` (interpolated lookup, input and output both 0-1)
4. If `DecibelMode` is on: convert value to dB range (-100 to 0), then convert dB to linear gain

Each stage feeds into the next. All three toggles are independent and can be combined in any combination.

### table-lookup-position: Where does the TableProcessor lookup occur?

The table receives the *potentially inverted* value. If both Inverted and UseTable are on, the table input is `(1.0 - velocity)`. The table's X axis domain is set to display as MIDI range (0-127) via `setXTextConverter(Modulation::getDomainAsMidiRange)`, but the actual lookup input is the 0-1 float. The table output is also 0-1.

### decibel-conversion-formula: What is the exact decibel conversion?

The formula is: `dB = -100 + 100 * value`, then `gain = decibelsToGain(dB)`. This maps the 0-1 input linearly to -100dB..0dB, then converts to linear gain. At value=0, gain is effectively 0 (silence). At value=1, gain is 1.0 (unity). The curve provides a perceptually more natural velocity response than the default linear mapping.

### description-accuracy-velocity-source: Does the module read velocity directly?

Yes. The velocity is read directly from the HiseEvent via `getFloatVelocity()`, which returns the MIDI velocity normalised to 0.0-1.0. No preprocessing occurs before the module's processing chain.

## Processing Chain Detail

1. **Velocity read** - Reads float velocity from HiseEvent. Per-voice, negligible CPU.
2. **Inversion** (conditional on Inverted) - Subtracts value from 1.0. Per-voice, negligible CPU.
3. **Table lookup** (conditional on UseTable) - Interpolated lookup in a 128-point table. Per-voice, negligible CPU.
4. **Decibel conversion** (conditional on DecibelMode) - Linear-to-dB mapping then dB-to-gain conversion. Per-voice, negligible CPU.

## Modulation Points

None. This module has no modulation chains. It is itself a modulation source.

## Conditional Behavior

All three parameters are independent toggles that gate processing stages:

- **Inverted = On**: Flips the velocity curve (velocity 127 -> 0.0, velocity 1 -> ~1.0)
- **UseTable = On**: Enables interpolated table lookup. The table editor appears in the UI. Input is the (potentially inverted) velocity, output replaces the value.
- **DecibelMode = On**: Applies logarithmic gain curve. Applied after table lookup, so the table operates in linear domain regardless of this setting.

All combinations of these three toggles are valid and compose predictably.

## Interface Usage

**TableProcessor (LookupTableProcessor):** The module creates one shared lookup table at construction. The table's X axis text converter is set to display MIDI velocity range (0-127). When UseTable is enabled, the table provides interpolated lookup from the velocity value (0-1 input, 0-1 output). The table is serialised only when UseTable is enabled.

## CPU Assessment

All processing stages are negligible cost - a single float read, optional subtraction, optional table lookup (small array interpolation), and optional dB conversion. This runs once per voice per note-on, not per sample.

- **Baseline:** negligible
- **Polyphonic:** true (per-voice)
- **Scaling factors:** none

## UI Components

The editor is `VelocityEditorBody`, a standard processor editor (not a FloatingTile). No dedicated FloatingTile content type exists for this module.

## Notes

The `inputValue` member variable is declared but not used in the current signal path - it appears to be leftover from an earlier implementation. This is cosmetic and has no effect on behaviour.
