# GlobalModulatorContainer - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/synthesisers/synths/GlobalModulatorContainer.h` (~923 lines)
- `hi_core/hi_modules/synthesisers/synths/GlobalModulatorContainer.cpp` (~870 lines)
- `hi_core/hi_modules/modulators/mods/GlobalModulators.h` (constrainer, base class)
- `hi_core/hi_modules/modulators/mods/GlobalModulators.cpp` (constrainer implementation)

**Base class:** `ModulatorSynth`

## Signal Path

GlobalModulatorContainer is a SoundGenerator that produces no audio. Its `calculateBlock()` fills the voice buffer with zeros. Its purpose is to host modulators in its GainModulation chain (renamed to "Global Modulators") and make their computed values available to consumer modules elsewhere in the module tree.

The signal flow is:
1. MIDI noteOn/noteOff events trigger voice allocation (phantom voices for polyphonic modulator context)
2. `preStartVoice()` captures voice-start values from all hosted VoiceStartModulators into per-note-number lookup arrays
3. `preVoiceRendering()` renders TimeVariantModulators and monophonic envelopes into shared buffers
4. Per-voice envelopes compute during the voice's `calculateBlock()` and their values are stored in event-indexed buffers
5. Consumer modules (GlobalVoiceStart/TimeVariant/Envelope/StaticTimeVariant) read from these shared buffers
6. Audio output is silence (zeros)

## Gap Answers

### container-purpose

**Question:** Does GlobalModulatorContainer produce any audio at all, or is renderNextBlock a no-op?

**Answer:** No audio output. The voice's `calculateBlock()` explicitly fills both channels with zeros via `FloatVectorOperations::fill()`. It also reports `getNumActiveVoices()` as 0. The GainChain is repurposed: set to `Modulation::GlobalMode` (values are copied to destination buffers rather than multiplied as gain), renamed to "Global Modulators". The PitchChain and EffectChain are disabled in the constructor.

Modulators live in the GainModulation chain at `BasicChains::GainChain` (chainIndex 1). When the chain changes, `refreshList()` builds three parallel data arrays: `voiceStartData`, `timeVariantData`, and `envelopeData`, each holding shared buffers for consumer modules.

### modulator-hosting

**Question:** How do child modulators get registered for global access?

**Answer:** When the modulation chain changes, `refreshList()` iterates the chain handler's active lists and builds typed data arrays:
- `voiceStartData` from `activeVoiceStartList`
- `timeVariantData` from `activeTimeVariantsList`
- `envelopeData` from `activeEnvelopesList` + `activeMonophonicEnvelopesList`

These `VoiceStartData`, `TimeVariantData`, and `EnvelopeData` structs store computed modulation values per-block in shared buffers. Consumer modules connect via string-based lookup with format `"ContainerId:ModulatorId"`. The `GlobalModulator` base class provides `connectToGlobalModulator()` which iterates all `GlobalModulatorContainer` instances via `Processor::Iterator`, matches by container ID, then finds the modulator by name. Consumer data retrieval methods include:
- `getConstantVoiceValue(modulator, noteNumber)` for VoiceStart consumers
- `getModulationValuesForModulator(modulator, startSample)` for TimeVariant consumers
- `getEnvelopeValuesForModulator(envelopeIndex, startSample, event)` for Envelope consumers

### constrainer-meaning

**Question:** What does the '!' prefix mean in '!Global*Modulator'?

**Answer:** The `NoGlobalsConstrainer` class excludes the four global consumer types from the chain. The `!` prefix means "negative filter" - these types are prohibited. The constrainer populates an `illegalTypes` list with `GlobalEnvelopeModulator`, `GlobalVoiceStartModulator`, `GlobalTimeVariantModulator`, and `GlobalStaticTimeVariantModulator`. The wildcard is generated via `makeNegativeFilterWildcard()`.

All other modulator types CAN be added: LFOs, velocity modulators, AHDSR envelopes, constant modulators, random modulators, script modulators, etc. The purpose is preventing circular references.

### gain-param-relevance

**Question:** Is the Gain parameter functional?

**Answer:** The Gain parameter is inherited from ModulatorSynth but functionally irrelevant. The chain mode is `GlobalMode` (not `GainMode`), the chain is renamed to "Global Modulators", and `calculateBlock()` outputs silence. The Gain slider has no audible effect. `getVoiceStartValueFor()` always returns 1.0. `synthNeedsEnvelope()` returns false. The Gain, Balance, VoiceLimit, and KillFadeTime parameters are all vestigial in terms of audio output.

However, VoiceLimit and KillFadeTime are functional at the voice allocation level - they control the phantom voices used for polyphonic modulator context.

### voice-architecture

**Question:** Does GlobalModulatorContainer participate in voice allocation?

**Answer:** Yes, fully. The constructor creates `numVoices` voices + 1 sound. `GlobalModulatorContainerSound` accepts all notes/channels/velocities. Voice allocation is essential because polyphonic modulators (envelopes, voice-start modulators) need per-voice state.

Key voice lifecycle:
- `preStartVoice()` saves voice-start values into per-note-number arrays
- `startNote()` starts envelope data tracking per voice event
- `checkRelease()` manages envelope lifecycle with a multi-phase clear state (PendingReset1 -> PendingReset2 -> Reset)
- `handleRetriggeredNote()` explicitly does NOT kill old voices to preserve envelope tails

### ui-components

**Question:** Does GlobalModulatorContainer have a custom editor or FloatingTile?

**Answer:** The container's `createEditor()` returns `EmptyProcessorEditorBody` - no custom editor. It relies on the standard ModulatorSynth processor editor to show the modulation chains.

No FloatingTile content type exists for the container itself. However, there is a `GlobalContainerMatrixModulationPopupData` class that integrates with `MacroControlledObject` sliders for drag-and-drop modulation assignment. The container also has a `DragAction` enum and `dragBroadcaster` for drag-based modulation connection UI.

## Processing Chain Detail

1. **Voice allocation** (negligible) - phantom voices track note lifecycle
2. **preStartVoice** (negligible) - saves voice-start modulator values to per-note arrays
3. **preVoiceRendering** (low) - renders time-variant and monophonic envelope modulators into shared buffers
4. **calculateBlock** (negligible) - zero-fills audio output, stores envelope values in event-indexed buffers
5. **checkRelease** (negligible) - manages envelope clear state with two-buffer delay

## Modulation Points

The "Global Modulators" chain (chainIndex 1) operates in `Modulation::GlobalMode`. Modulation values are copied to destination buffers rather than applied as gain multiplication. The chain serves as a pure container for source modulators.

## Vestigial / Notable

- **Gain parameter**: Inherited from ModulatorSynth, vestigial for audio. The chain it targets is repurposed as a modulator container.
- **Balance parameter**: Inherited, vestigial. No audio to balance.
- **VoiceLimit/KillFadeTime**: Functional for voice allocation of phantom voices, but not for audio output.
- **Pitch Modulation chain**: Explicitly disabled in constructor.

## CPU Assessment

**Negligible baseline cost.** The container itself does no audio DSP. The real work is done by the hosted modulators (which would compute anyway if placed locally). The global system actually saves CPU by computing once and sharing across multiple consumer targets. Consumer modules simply copy from pre-computed buffers.

## UI Components

- No dedicated FloatingTile
- `EmptyProcessorEditorBody` for the container itself
- `GlobalContainerMatrixModulationPopupData` for drag-and-drop modulation assignment
- Consumer modules use `GlobalModulatorEditor` with a ComboBox dropdown

## Notes

The GlobalModulatorContainer is the producer half of HISE's global modulation system. It is a SoundGenerator by inheritance but functions purely as a modulation routing hub. Its phantom voice allocation is a clever architectural choice that gives hosted polyphonic modulators the voice context they need without producing any audio. The two-phase release clear state (PendingReset1 -> PendingReset2 -> Reset) ensures envelope release tails are not cut off prematurely.
