# GlobalEnvelopeModulator - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/modulators/mods/GlobalModulators.h:258-305` (class definition)
- `hi_core/hi_modules/modulators/mods/GlobalModulators.cpp:626-842` (implementation)
- `hi_core/hi_modules/modulators/editors/GlobalModulatorEditor.h` (shared editor)

**Base class:** `EnvelopeModulator` + `GlobalModulator`

## Signal Path

GlobalEnvelopeModulator reads per-voice envelope values from a source envelope modulator hosted in a GlobalModulatorContainer. Voice synchronisation uses event IDs, not voice indices.

1. User selects source via ComboBox dropdown (format: "ContainerId:ModulatorId")
2. On `startVoice()`: captures the current HiseEvent, looks up the envelope index in the container
3. On `calculateBlock()`: retrieves the source envelope's pre-computed buffer for this voice's event, copies it into the local buffer
4. Optional table lookup transforms the values per-sample
5. Inversion is NOT functional (code is commented out)
6. `isPlaying()` delegates to the container to check if the source envelope is still active for this event

## Gap Answers

### signal-flow

**Question:** How does this module receive the envelope value from the GlobalModulatorContainer?

**Answer:** In `calculateBlock()`, the consumer calls `getConnectedContainer()->getEnvelopeValuesForModulator(envelopeIndex, startSample, currentEvent)`. The container looks up the `EnvelopeData` entry by index and returns a pointer to the pre-computed buffer for the matching event. The event is mapped to a buffer slot via `eventId % NUM_POLYPHONIC_VOICES`. The consumer then copies this buffer into its own `internalBuffer` via `FloatVectorOperations::copy()` or per-sample table lookup.

### connection-mechanism

**Question:** How does the user select which global envelope to connect to?

**Answer:** A `ComboBox` dropdown in `GlobalModulatorEditor` lists all available envelope modulators from all GlobalModulatorContainers. The list is populated by `getListOfAllModulatorsWithType()` which iterates all containers and collects envelopes. The connection string format is `"ContainerId:ModulatorId"`, stored as the "Connection" property in the ValueTree. Deferred connection handles load-order issues via `pendingConnection`.

### table-application

**Question:** Where is the table lookup applied?

**Answer:** The table is applied per-sample in `calculateBlock()` when `useTable` is true. For each sample, the raw source value is passed through `getTableUnchecked(0)->getInterpolatedValue()`. Input range is 0-1, output defined by the table. The table is applied to the source envelope values before any other transformation.

### voice-sync

**Question:** How does voice synchronisation work?

**Answer:** Voice sync uses the HiseEvent mechanism, not direct voice index mapping. At `startVoice()`, the consumer captures the HiseEvent from its owner voice. In `calculateBlock()`, this event is passed to the container's `getEnvelopeValuesForModulator()`, which maps the event to a buffer slot via `eventId % NUM_POLYPHONIC_VOICES`. The source envelope's data was saved to the same event-indexed slot during rendering. This allows correct behaviour even when consumer and source synths have different voice allocation. For monophonic source envelopes, the index is always 0.

### monophonic-retrigger

**Question:** Are Monophonic and Retrigger functional?

**Answer:** Both are inherited from `EnvelopeModulator` and `setInternalAttribute()` delegates indices below `EnvelopeModulator::Parameters::numParameters` to the base class. Monophonic is technically functional at the framework level (affects voice allocation), but the actual envelope shape comes from the source. Retrigger is handled by the base `EnvelopeModulator::startVoice()` framework. In practice, these parameters affect the consumer's voice management but not the envelope shape, which is always determined by the source.

### disconnected-behavior

**Question:** What happens when no global envelope is connected?

**Answer:** The buffer is filled with `Modulation::getInitialValue()`, which is 1.0 for gain mode and 0.0 for pitch mode. If the container returns a null pointer (source data not available), the buffer is filled with 0.0.

## Processing Chain Detail

1. **startVoice** (negligible) - captures HiseEvent, looks up envelope index
2. **calculateBlock** (low-medium) - copies source buffer or applies per-sample table lookup
3. **isPlaying** (negligible) - delegates to container's envelope clear state check
4. **stopVoice** (negligible) - marks voice inactive

## Modulation Points

No modulation chains of its own. Receives modulation values from the source envelope in the GlobalModulatorContainer.

## Vestigial / Notable

- **Inverted parameter**: Defined but has no effect. The inversion code is commented out in both the table and non-table paths of `calculateBlock()`.
- **Monophonic/Retrigger**: Inherited from EnvelopeModulator base. Affect voice management framework but not envelope shape.

## CPU Assessment

**Low baseline cost.** Per-block buffer copy from pre-computed source data. Optional per-sample table lookup adds moderate cost. No DSP computation of its own - purely a proxy.

## UI Components

- `GlobalModulatorEditor` with ComboBox for source selection, UseTable toggle, TableEditor, and Invert toggle
- No dedicated FloatingTile

## Notes

The voice synchronisation via event IDs rather than voice indices is architecturally elegant - it decouples the consumer from the source's voice allocation. The two-phase clear state in the container prevents premature envelope cutoff. The commented-out inversion code suggests this was either intentionally disabled or is a bug that was never addressed.
