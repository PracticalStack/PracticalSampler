# Random Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/RandomModulator.h`, `hi_core/hi_modules/modulators/mods/RandomModulator.cpp`
**Base class:** `VoiceStartModulator`, `LookupTableProcessor`

## Signal Path

The signal path is a minimal two-stage chain:

noteOn trigger -> generate random float (0-1) -> [optional table lookup] -> modulation output (0-1)

All processing happens in `calculateVoiceStartValue()`, which is called once per voice on note-on via the base class `handleHiseEvent()`. The HiseEvent parameter is unused (the module generates its own value, ignoring MIDI data).

## Gap Answers

### signal-path-order: What is the exact processing order in calculateVoiceStartValue()?

The order in `calculateVoiceStartValue()` is:

1. Generate a random float via `generator.nextFloat()` (produces a value in [0.0, 1.0))
2. If `useTable` is true: replace the random value with `getTableUnchecked()->getInterpolatedValue(randomValue, sendNotificationAsync)`, which uses the random value as the X position for a linearly-interpolated lookup in the 512-entry table
3. Return the value as the modulation output

There is no additional processing. The HiseEvent parameter is not read at all.

### rng-algorithm: What random number generator is used?

The module uses JUCE's `juce::Random` class, stored as a member `generator`. It is seeded at construction time with `Time::currentTimeMillis()`. The `nextFloat()` method returns a uniformly distributed float in the range [0.0, 1.0). JUCE's Random uses a linear congruential generator internally. Each voice-start call advances the RNG state by one step, so the sequence is deterministic for a given seed but the seed itself varies per instantiation.

### table-lookup-application: When UseTable is enabled, does the random value serve as the X position?

Yes. The random value (0.0-1.0) is passed directly as the `sampleIndex` argument to `getInterpolatedValue()`. Inside that method, the input is scaled to the 512-entry table range, and a linearly interpolated value is returned. The table output replaces the random value entirely, so the table's Y axis defines the final modulation output. This effectively allows the user to remap the uniform distribution into a custom probability curve: flat table regions concentrate probability, steep regions spread it.

### usetable-conditional: When UseTable is Off, is the raw random value passed through directly?

Yes. When `useTable` is false, the `if (useTable)` branch is skipped entirely and the raw `generator.nextFloat()` value is returned unmodified. The table still exists in memory (allocated at construction via `LookupTableProcessor(mc, 1)`) but is not consulted. The default table state is a linear ramp (identity mapping), so enabling the table with default settings produces the same output as having it disabled.

## Processing Chain Detail

1. **Random generation** - Calls `generator.nextFloat()` to produce a uniform random float in [0, 1). Per-voice, negligible CPU.
2. **Table lookup** (conditional on UseTable) - Interpolated lookup in a 512-entry table. The random value is the X position (0-1 input), the table Y value is the output (0-1 output). Per-voice, negligible CPU.

## Modulation Points

None. This module has no modulation chains. It is itself a modulation source.

## Conditional Behavior

- **UseTable = Off (default):** Raw uniform random value passes through directly as modulation output.
- **UseTable = On:** The random value is remapped through the lookup table. The table's curve determines the effective probability distribution of the output. The table editor appears in the UI.

## Interface Usage

**TableProcessor (LookupTableProcessor):** The module creates one shared lookup table at construction (512 entries). When UseTable is enabled, the table provides linearly interpolated lookup from the random value (0-1 input, 0-1 output). The table is serialised as "RandomTableData" in the value tree. The header comment mentions "values are limited to 7bit for MIDI feeling" when using the table, but this is a documentation-era comment - the actual implementation uses full float precision with 512-point interpolation. There is no 7-bit quantisation in the code.

## CPU Assessment

Both processing stages are negligible cost - a single RNG call and an optional small-array interpolated lookup. This runs once per voice per note-on, not per sample.

- **Baseline:** negligible
- **Polyphonic:** true (per-voice)
- **Scaling factors:** none

## UI Components

The editor is `RandomEditorBody`, a standard processor editor (not a FloatingTile). No dedicated FloatingTile content type exists for this module.

## Notes

The header comment states that when the table is used, "values are limited to 7bit for MIDI feeling." This is inaccurate for the current implementation - the table uses 512-point interpolation with full float precision. The comment appears to be vestigial documentation from an earlier version.

The `currentValue` member variable is declared as `volatile float` in the header but is never read or written anywhere in the implementation. It appears to be a vestigial member from a previous implementation.
