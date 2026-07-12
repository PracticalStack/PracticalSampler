# MacroModulationSource - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/synthesisers/synths/MacroModulationSource.h` (145 lines)
- `hi_core/hi_modules/synthesisers/synths/MacroModulationSource.cpp` (178 lines)

## Gap Answers

### signal-flow

**Question:** How do the macro chains drive the macro control system?

**Answer:** The signal flow is entirely in `preVoiceRendering()` (MacroModulationSource.cpp:116-155):

1. The base class `ModulatorSynth::preVoiceRendering()` is called first, which renders all modulation chains.
2. Plugin parameter updates are suppressed via `ScopedValueSetter` (line 121) to prevent feedback loops.
3. For each macro chain (indexed from chain offset 2):
   - Skip chains that shouldn't be processed (`shouldBeProcessedAtAll()` returns false when the chain is empty).
   - Expand monophonic modulation values to audio rate (line 133).
   - Read the monophonic modulation value at the first sample (line 136-137).
   - Read the voice/constant modulation value (lines 140-145).
   - Multiply them together to get the final value `v`.
   - If `v` differs from `lastValues[i]`, call `getMainController()->getMainSynthChain()->setMacroControl(i, v * 127.0f, sendNotificationAsync)` (line 148).
   - Store `v` in `lastValues[i]` and update the display value (lines 150-152).

The macro value is scaled to 0-127 range before being sent to the macro system. Only changed values trigger updates (delta detection at line 147).

### vestigial-params

**Question:** Are the inherited parameters functional or vestigial?

**Answer:** All four inherited parameters are vestigial:

- **Gain** and **Balance**: The voice's `calculateBlock()` (line 172-175) is completely empty - it produces no audio. Gain and Balance are only applied by the base class to audio output, which is silent.
- **VoiceLimit** and **KillFadeTime**: The voice model exists but produces no audio. `getNumActiveVoices()` returns 0 (line 79). `synthNeedsEnvelope()` returns false (line 130).

The Gain and Pitch modulation chains are explicitly disabled in the constructor (lines 93-94). The FX chain is also disabled (line 95).

### voice-behavior

**Question:** What do the voices do?

**Answer:** The voices are structural overhead. `MacroModulationSourceVoice::calculateBlock()` (line 172-175) is completely empty - no audio processing at all. `startNote()` (line 164-169) initializes the voice with zero velocity and sets up basic uptime tracking, but the voice never produces audio.

The module does need at least one voice to trigger the `preVoiceRendering()` callback through the normal synth rendering pipeline. But `getNumActiveVoices()` is hardcoded to return 0 (line 79), signaling that no voices are actually producing audio.

### macro-chain-rendering

**Question:** How are the macro chain values sampled?

**Answer:** In `preVoiceRendering()`, only the first sample of each chain's output is used:

- `getMonophonicModulationValues(startSample)` returns a pointer, and only `m[0]` is read (line 137).
- `getWritePointerForManualExpansion(startSample)` also uses only `vv[0]` (line 142).
- If no voice modulation is available, `getConstantModulationValue()` is used (line 145).

So despite calling `expandMonophonicValuesToAudioRate()` (line 133), only the first sample is actually consumed. The update rate is effectively per-block (once per audio buffer), not per-sample.

The chains have `setExpandToAudioRate(true)` and `setIncludeMonophonicValuesInVoiceRendering(true)` (lines 76-77) configured in the constructor, which ensures monophonic modulators are properly rendered even without active voices.

### macro-count

**Question:** Is the number of macro chains fixed at 8 or configurable?

**Answer:** The number of macro chains is determined by `HISE_NUM_MACROS`, a preprocessor define that defaults to 8 (`hi_core.h:315`) but can be configured per-project up to `HISE_NUM_MAX_MACROS` (64). The value is read at runtime via `HISE_GET_PREPROCESSOR(getMainController(), HISE_NUM_MACROS)` (line 60), so the chain count matches the project's macro count setting.

The `lastValues` array is sized to `HISE_NUM_MAX_MACROS` (64) to accommodate any configuration (MacroModulationSource.h:134).

## Additional Findings

- The constructor creates voices and a sound but they serve no audio purpose - the module needs the synth infrastructure to participate in the rendering pipeline and trigger `preVoiceRendering()`.
- The editor is `EmptyProcessorEditorBody` (line 107) - no custom UI. The macro chains appear as child modulation chain slots.
- `addProcessorsWhenEmpty()` is overridden to do nothing (line 126) - no default child processors are added.
- The `Chain::Handler::Listener` interface is implemented but `processorChanged()` is empty (lines 81-84).
- The macro value sent to the system is `v * 127.0f` - the 0-1 modulation output is scaled to MIDI-style 0-127 range.

## Issues

No description inaccuracies found. All base data descriptions match the C++ implementation.
