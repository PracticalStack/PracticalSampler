# GlobalStaticTimeVariantModulator - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/modulators/mods/GlobalModulators.h:188-256` (class definition)
- `hi_core/hi_modules/modulators/mods/GlobalModulators.cpp:562-624` (implementation)
- `hi_core/hi_modules/modulators/editors/GlobalModulatorEditor.h` (shared editor)

**Base class:** `VoiceStartModulator` + `GlobalModulator`

## Signal Path

GlobalStaticTimeVariantModulator captures the last computed value of a source TimeVariantModulator at voice start time. Despite connecting to a continuous (TimeVariant) source, it behaves as a VoiceStartModulator - returning a single per-voice value frozen at note-on.

1. User selects source via ComboBox dropdown (format: "ContainerId:ModulatorId")
2. On `calculateVoiceStartValue()`: calls `getConnectedContainer()->getTimeVariantModulatorValuesForModulator(connectedMod)` then reads `.getLastConstantValue()` from the source
3. Optional table lookup transforms the captured value
4. Inversion applied (1 - value)
5. Result returned as the voice start modulation value

## Gap Answers

### signal-flow

**Question:** What is the complete signal flow in calculateVoiceStartValue? How does this module capture the current value of a TimeVariantModulator at voice start time?

**Answer:** In `calculateVoiceStartValue()`, the consumer retrieves the source `TimeVariantModulator` pointer via the container's `TimeVariantData` lookup. It then calls `getLastConstantValue()` on the source modulator. This returns the last value the source computed during its most recent `calculateBlock()` - it is NOT interpolated to the exact note-on timestamp. The value is then optionally transformed via table lookup and inversion.

### connection-mechanism

**Question:** How does the user select which global time variant modulator to connect to? Same mechanism as other Global*Modulator consumers?

**Answer:** Same mechanism as all GlobalModulator consumers. The ComboBox lists TimeVariantModulators (not VoiceStartModulators, despite this module being a VoiceStartModulator subclass). The connection string is `"ContainerId:ModulatorId"`. The connection targets a TimeVariant source, which is what makes this module a unique hybrid.

### table-application

**Question:** Where is the table lookup applied? Before or after inversion? What is the input/output range?

**Answer:** The table is applied before inversion, same as GlobalVoiceStartModulator. The captured value is passed through `getTableUnchecked(0)->getInterpolatedValue()` when `useTable` is true. The result is then inverted if `Inverted` is enabled. Input range is 0-1 (last constant value from source), output defined by the table curve.

### snapshot-timing

**Question:** At what exact point is the source TimeVariantModulator's value captured? Is it the last calculated block value, or is the source evaluated specifically for this voice start event?

**Answer:** The value is the last calculated block value via `getLastConstantValue()`. This returns whatever the source TimeVariantModulator produced in its most recent `calculateBlock()` call - typically the last sample of the previous audio block. It is NOT evaluated specifically for this note-on event. For fast-moving sources like LFOs, the captured value depends on the timing relationship between the note-on and the audio block boundaries, introducing inherent quantisation to block size.

### disconnected-behavior

**Question:** What happens when no global time variant modulator is connected?

**Answer:** When disconnected, `calculateVoiceStartValue()` returns 1.0 (unity/pass-through for gain mode). The `isConnected()` check gates all processing.

## Processing Chain Detail

1. **calculateVoiceStartValue** (negligible) - single value read from source's last constant, optional table + inversion

## Modulation Points

No modulation chains of its own. Captures a single value from a TimeVariantModulator source in the GlobalModulatorContainer.

## Vestigial / Notable

- **Inverted parameter**: Functional. Applied as `1.0f - value` after table lookup.
- The block-quantised capture is an inherent design limitation, not a bug. Users should be aware that fast-moving LFO sources will produce values quantised to the audio block rate.

## CPU Assessment

**Negligible cost.** Single value read per voice start event. No per-block processing.

## UI Components

- `GlobalModulatorEditor` with ComboBox for source selection, UseTable toggle, TableEditor, and Invert toggle
- No dedicated FloatingTile

## Notes

This module fills a unique niche: it bridges the continuous (TimeVariant) and per-voice (VoiceStart) modulation domains. By freezing an LFO or other continuous modulator's value at note-on, each voice gets a different static value based on when it was triggered. This is useful for pseudo-random per-voice variation from a shared LFO source. The block-quantised capture is coarser than true sample-accurate snapshot but is sufficient for most musical applications.
