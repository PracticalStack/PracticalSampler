# Notenumber Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/KeyModulator.h`, `hi_core/hi_modules/modulators/mods/KeyModulator.cpp`
**Base class:** `VoiceStartModulator`, `LookupTableProcessor`

## Signal Path

The signal path is a single-step transformation with no conditionals:

noteOn note number (0-127) -> normalize (/ 127.0) -> table lookup -> modulation output (0-1)

All processing happens in `calculateVoiceStartValue()`, which is called once per voice on note-on. The table lookup is unconditional -- there is no UseTable toggle. The table is always active.

## Gap Answers

### signal-path-order: How is the MIDI note number mapped to 0-1 modulation output?

The entire processing chain is a single expression in `calculateVoiceStartValue()`:

`return getTableUnchecked(0)->getInterpolatedValue(m.getNoteNumber() / 127.0, sendNotificationAsync);`

1. Read integer note number from the HiseEvent via `getNoteNumber()` (0-127)
2. Normalize to 0-1 by dividing by 127.0
3. Pass the normalized value as the X position into the lookup table
4. Return the table's interpolated output as the modulation value

The division is by 127.0 (not 128.0), so MIDI note 0 maps to exactly 0.0 and MIDI note 127 maps to exactly 1.0. The default table is a linear identity ramp, so without user editing, the output equals the normalized note number.

### table-lookup-position: Where does the TableProcessor lookup occur and is it gated?

The table lookup is the only processing stage and is always active. There is no UseTable toggle -- unlike Velocity and Random which have a UseTable parameter that gates the table path, KeyNumber always routes through the table. The table receives the normalized note number (0-1) as its X position. The table's X axis display is configured in the constructor via `setXTextConverter(Modulation::getDomainAsMidiNote)`, which shows MIDI note names (e.g., C3, F#4) rather than raw numbers.

### usetable-parameter-existence: Does KeyNumber have a UseTable parameter?

No. KeyNumber has zero parameters. The `setInternalAttribute()` method body is empty, and `getAttribute()` returns a hardcoded `0.0f`. There is no Parameters enum. The table is always active -- the module's sole purpose is to map note numbers through the table curve.

This is a deliberate design choice that differs from Velocity: since the table IS the module's functionality (there is no meaningful "raw note number" modulation without it), a UseTable toggle would be redundant.

### note-number-range: Does the module use the full 0-127 MIDI note range?

Yes. The normalization is `noteNumber / 127.0` with no clamping, range restriction, or special handling. MIDI note 0 maps to X=0.0 and MIDI note 127 maps to X=1.0. All 128 note values are distributed linearly across the full table domain.

## Processing Chain Detail

1. **Note number read** - Reads integer note number from HiseEvent. Per-voice, negligible CPU.
2. **Normalization** - Divides by 127.0 to produce 0-1 range. Per-voice, negligible CPU.
3. **Table lookup** - Interpolated lookup in the SampleLookupTable. Per-voice, negligible CPU. Always active (no toggle).

## Modulation Points

None. This module has no modulation chains. It is itself a modulation source.

## Conditional Behavior

None. There are no parameters and no conditional branches. The processing is a fixed pipeline.

## Interface Usage

**LookupTableProcessor (TableProcessor):** The module inherits from `LookupTableProcessor` with a single table (index 0). The table is the core of the module's functionality -- it maps the normalized note number (0-1) to the output value (0-1). The constructor sets the X axis text converter to `Modulation::getDomainAsMidiNote` so the table editor displays MIDI note names on the X axis.

The table data is serialized under the key "MidiTableData" in `restoreFromValueTree`/`exportAsValueTree`.

## Vestigial / Notable

The class Doxygen comment says "A constant Modulator which calculates a random value at the voice start" -- this is copy-pasted from RandomModulator and does not describe KeyModulator. The actual behavior is note-number-to-modulation mapping.

The KeyEditor's Jucer metadata (in the `#if 0` block at the bottom of KeyEditor.cpp) references `KeyModulator::NumberMode` and `KeyModulator::KeyMode` enum values that do not exist in the current header. The active editor code does not use these -- it connects a single TableEditor via `ProcessorHelpers::connectTableEditor`. The Jucer metadata is vestigial from an older version that had two editing modes (a continuous table and a discrete per-key editor).

## CPU Assessment

Negligible. One integer read, one floating-point division, one interpolated table lookup. All per-voice, once at note-on only. No per-sample or per-block processing.

## UI Components

The editor (`KeyEditor`) creates a single `TableEditor` widget connected to the module's table via `ProcessorHelpers::connectTableEditor`. No FloatingTile content types were found.

## Notes

KeyNumber is structurally simpler than Velocity despite sharing the same base classes. Velocity has three parameters (Inverted, UseTable, DecibelMode) that gate processing stages; KeyNumber has zero parameters and the table is always active.

The forum insights note that the Array Modulator is the preferred alternative for per-note value mapping, as recommended by the HISE author. KeyNumber provides a continuous curve (interpolated table), while ArrayModulator provides discrete per-note values.
