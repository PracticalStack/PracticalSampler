# GlobalVoiceStartModulator - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/modulators/mods/GlobalModulators.h:93-137` (class definition)
- `hi_core/hi_modules/modulators/mods/GlobalModulators.cpp:220-390` (implementation)
- `hi_core/hi_modules/modulators/editors/GlobalModulatorEditor.h` (shared editor)

**Base class:** `VoiceStartModulator` + `GlobalModulator`

## Signal Path

GlobalVoiceStartModulator reads a per-note voice start value from a source VoiceStartModulator hosted in a GlobalModulatorContainer. The value is looked up by MIDI note number, not voice index.

1. User selects source via ComboBox dropdown (format: "ContainerId:ModulatorId")
2. On `calculateVoiceStartValue()`: retrieves the pre-computed value from the container's 128-element `voiceValues` array using the note number from the HiseEvent
3. Optional table lookup transforms the value
4. Inversion applied (1 - value)
5. Result returned as the voice start modulation value

## Gap Answers

### signal-flow

**Question:** What is the complete signal flow in calculateVoiceStartValue? How does this module retrieve the value from the source VoiceStartModulator in the GlobalModulatorContainer?

**Answer:** In `calculateVoiceStartValue()`, the consumer calls `getConnectedContainer()->getConstantVoiceValue(connectedMod, noteNumber)`. The container looks up the `VoiceStartData` entry matching the source modulator and returns the value from its `voiceValues[noteNumber]` array - a 128-element float array indexed by MIDI note number. This array was populated during the container's `preStartVoice()` when the note was triggered. The consumer then optionally applies table lookup via `getTableUnchecked(0)->getInterpolatedValue()` and inversion via `1.0f - value`.

### connection-mechanism

**Question:** How does the user select which global voice start modulator to connect to?

**Answer:** Same mechanism as all GlobalModulator consumers. A `ComboBox` dropdown in `GlobalModulatorEditor` lists all available VoiceStartModulators from all GlobalModulatorContainers. The list is populated by `getListOfAllModulatorsWithType()` which iterates all containers and collects VoiceStartModulators. The connection string format is `"ContainerId:ModulatorId"`, stored as the "Connection" property in the ValueTree. Deferred connection handles load-order issues via `pendingConnection`.

### table-application

**Question:** Where is the table lookup applied? Before or after inversion? What is the input/output range?

**Answer:** The table is applied before inversion. In `calculateVoiceStartValue()`, the raw source value is passed through `getTableUnchecked(0)->getInterpolatedValue()` when `useTable` is true. The result is then inverted if `Inverted` is enabled. Input range is 0-1 (modulation value from source), output defined by the table curve.

### voice-mapping

**Question:** How does voice mapping work? When this module is asked for a voice start value, does it query the source modulator for the same voice index?

**Answer:** Voice mapping uses MIDI note number, not voice index. The container stores voice-start values in a 128-element array indexed by note number during `preStartVoice()`. The consumer retrieves the value using the note number from the current HiseEvent. This means all voices playing the same note number get the same voice-start value, which is correct for modulators like velocity (same for all voices on a given note).

### disconnected-behavior

**Question:** What happens when no global voice start modulator is connected?

**Answer:** When disconnected (no connected container or modulator), `calculateVoiceStartValue()` returns 1.0 (unity/pass-through for gain mode). The `isConnected()` check at the top of the method gates all processing.

## Processing Chain Detail

1. **calculateVoiceStartValue** (negligible) - single array lookup by note number, optional table + inversion

## Modulation Points

No modulation chains of its own. Receives a single value from the source VoiceStartModulator in the GlobalModulatorContainer.

## Vestigial / Notable

- **Inverted parameter**: Functional. Applied as `1.0f - value` after table lookup.
- All processing is a single value lookup per voice start - no buffer operations.

## CPU Assessment

**Negligible cost.** Single array lookup per voice start event. Optional table lookup adds trivial cost. No per-block processing.

## UI Components

- `GlobalModulatorEditor` with ComboBox for source selection, UseTable toggle, TableEditor, and Invert toggle
- No dedicated FloatingTile

## Notes

The note-number-based lookup (rather than voice index) is a clean design that avoids cross-synth voice mapping complexity. The 128-element array means values are effectively cached per note number, so rapid retriggering of the same note returns the same modulation value from the source.
