# Module Pipeline Issues

Bugs, design issues, and silent failures discovered during C++ signal flow exploration (Phase 1).
Sorted by severity (critical first).

**Types:** silent-fail, missing-validation, inconsistency, code-smell, ux-issue, vestigial
**Severity:** critical, high, medium, low

---

## Critical

(No issues yet.)

## High

### TableEnvelope -- Monophonic releaseModValue reads from attackChain instead of releaseChain

- **Type:** silent-fail
- **Severity:** high
- **Location:** hi_core/hi_modules/modulators/mods/TableEnvelope.cpp:158
- **Observed:** In `startVoice()`, the monophonic path sets `monoState->releaseModValue = 1.0f / jmax<float>(0.001f, attackChain->getConstantVoiceValue(voiceIndex))`. This reads from `attackChain` instead of `releaseChain`. The polyphonic path at line 184 correctly uses `releaseChain->getConstantVoiceValue(voiceIndex)`. As a result, in monophonic mode, the release time modulation is driven by the attack modulation chain's value, not the release modulation chain.
- **Expected:** Line 158 should read `monoState->releaseModValue = 1.0f / jmax<float>(0.001f, releaseChain->getConstantVoiceValue(voiceIndex));` to match the polyphonic path.

### CC2Note -- Bypass parameter description is misleading

- **Type:** inconsistency
- **Severity:** high
- **Location:** hi_scripting/scripting/HardcodedScriptProcessor.h:538-559
- **Observed:** The Bypass parameter metadata description says "Bypasses CC-to-note triggering", but the `Synth.playNote()` call at line 559 is outside the bypass guard (lines 538-555). Bypass only toggles between custom round-robin group cycling (bypass OFF) and the Sampler's built-in round-robin (bypass ON, via `Sampler.enableRoundRobin(true)` in `onControl()` at line 570). Notes are always generated from CC regardless of bypass state.
- **Expected:** Either: (a) move the `Synth.playNote()` call inside the bypass guard so that bypass actually stops note generation, or (b) update the metadata description to "Switches between custom paired group cycling and the Sampler's built-in round-robin" to accurately reflect the behavior.

## Medium

### Delay -- tempoChanged() assigns syncTimeLeft to both channels

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Delay.h:76
- **Observed:** In `tempoChanged()`, line 76 computes `delayTimeRight = TempoSyncer::getTempoInMilliSeconds(newTempo, syncTimeLeft)` using `syncTimeLeft` instead of `syncTimeRight`. This writes an incorrect value to `delayTimeRight`. However, `calcDelayTimes()` is called immediately after on line 78, which recalculates the actual delay times from the correct `syncTimeLeft`/`syncTimeRight` values and calls `setDelayTimeSeconds()` on both delay lines. The cached `delayTimeRight` member is only read back by `getAttribute(DelayTimeRight)` in the non-tempo-synced branch, so the wrong value may never surface in practice (the variable is only meaningful when `tempoSync == false`, but `tempoChanged` only runs when `tempoSync == true`).
- **Expected:** Line 76 should use `syncTimeRight` instead of `syncTimeLeft`: `delayTimeRight = TempoSyncer::getTempoInMilliSeconds(newTempo, syncTimeRight);`

### Delay -- LowPassFreq and HiPassFreq parameters are vestigial

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Delay.h:56-57, Delay.cpp:71-80
- **Observed:** `LowPassFreq` and `HiPassFreq` are defined in the `Parameters` enum (indices 4 and 5), stored as member variables, serialized in `restoreFromValueTree`/`exportAsValueTree`, exposed in `getAttribute`/`setInternalAttribute`, and registered in `createMetadata()` with descriptions claiming they are "applied to the delay feedback". However, no filter objects exist in the class and `applyEffect()` does not reference these values anywhere. The delay editor also does not expose sliders for them. The parameters are fully wired for persistence and metadata but have zero DSP effect.
- **Expected:** Either implement the filters (add filter objects and apply them in the feedback path as the metadata descriptions claim) or remove the parameters from the enum and metadata to avoid confusion.

### AHDSR -- EcoMode parameter is vestigial

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/AhdsrEnvelope.cpp:441
- **Observed:** The `EcoMode` parameter (index 10) is defined in the enum, serialized, exposed in metadata with description "Enables 16x downsampling for reduced CPU usage", and defaults to 1.0 (enabled). However, `setInternalAttribute` handles it with `case EcoMode: break; // not needed anymore...` and `getAttribute` returns a hardcoded `1.0f`. The actual control rate downsampling is now handled globally by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` (8x for instruments, 1x for effects), making EcoMode redundant.
- **Expected:** Either remove the parameter from the enum and metadata, or document in the metadata description that it has no effect and downsampling is controlled globally.

### GlobalTimeVariantModulator -- Inverted parameter has no effect when UseTable is enabled

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/GlobalModulators.cpp (calculateBlock table path)
- **Observed:** In `calculateBlock()`, when `useTable` is true, the while loop uses `--numSamples` as both the loop condition and the per-sample counter, decrementing it to 0 or -1. After the loop, `invertBuffer(internalBuffer, numSamples)` is called, but `numSamples` is now <= 0, so the guard condition in `invertBuffer()` prevents execution. Inversion never applies in the table path. The non-table path works correctly because `numSamples` is not consumed before `invertBuffer()` is called.
- **Expected:** Save `numSamples` to a local variable before the table loop, then pass the original count to `invertBuffer()`. Alternatively, restructure the loop to not consume `numSamples`.

### GlobalEnvelopeModulator -- Inverted parameter is vestigial (inversion code commented out)

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/GlobalModulators.cpp (calculateBlock)
- **Observed:** The `Inverted` parameter is defined in the enum, exposed in the editor UI, and serialized, but the inversion code in `calculateBlock()` is commented out in both the table and non-table paths. The parameter has no effect on the modulation output despite being visible to users.
- **Expected:** Either uncomment and verify the inversion code, or remove the Inverted parameter from the enum and editor to avoid user confusion.

### SimpleReverb -- DryLevel parameter is vestigial (slaved to WetLevel)

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/SimpleReverb.h:99-101
- **Observed:** In `setInternalAttribute()`, setting WetLevel (case WetLevel) also sets `parameters.dryLevel = 1.0f - newValue`. Setting DryLevel (case DryLevel) contains only `break;` - it does nothing. The DryLevel slider appears in the UI and its value is serialized, but changing it has no effect on the audio output. WetLevel fully controls both wet and dry levels.
- **Expected:** Either make DryLevel independently functional (remove the automatic `dryLevel = 1 - wetLevel` coupling), or remove the DryLevel parameter from the enum and metadata and rename WetLevel to "Mix" to clarify that it controls the wet/dry balance.

### ShapeFX -- Drive parameter is vestigial (not read in applyEffect)

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/ShapeFX.cpp:212, ShapeFX.cpp:574-665
- **Observed:** The `Drive` parameter (index 10) is defined in the enum, stored as a member variable (`drive`), serialised in `exportAsValueTree`/`restoreFromValueTree`, and exposed in `createMetadata()` with description "Drive amount applied to the shaper input". However, in `setInternalAttribute()` the handler is `case Drive: drive = newValue; break;` with no call to `updateMode()` or `updateGain()`, and the `drive` member is never read in `applyEffect()`. The only input gain applied is via `gainer.processBlock()` which uses the `gain` member (set from the Gain parameter). Drive IS functional in the polyphonic variant PolyshapeFX where it multiplies the audio signal per-sample.
- **Expected:** Either connect Drive to the DSP path (multiply into the signal before shaping, similar to PolyshapeFX), or remove it from the metadata to avoid user confusion. The parameter currently appears in the UI but does nothing.

### HarmonicFilter -- Non-SSE filter path collapses stereo to mono

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/HarmonicFilter.h:119
- **Observed:** In `PeakFilterBand::State::process(FloatType& left, FloatType& right)` (the non-SSE path), line 119 sets `right = left` after processing, meaning the right channel always receives a copy of the left channel's filtered output. The SSE path (`process(SSEType& input)`) processes both channels independently via SIMD. On non-SSE builds, the HarmonicFilter produces mono output even when given stereo input. The `processSamples()` method passes separate left and right pointers, but the scalar `process()` ignores the right channel's input and overwrites it.
- **Expected:** The non-SSE path should process each channel independently (duplicate the SVF state for L/R or process them in separate calls), matching the SSE path's behavior.

### HarmonicFilter -- Monophonic version produces silence with more than 1 band

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/HarmonicFilter.cpp (monophonic / MasterEffect path)
- **Observed:** Using the monophonic (MasterEffect) version of the HarmonicFilter with NumFilterBands set to more than 1 results in complete silence. The polyphonic (VoiceEffect) version works correctly with any band count. Multiple users confirmed independently (tid 1213, tid 1150). On Linux with JACK, this can silence the entire audio system, not just the HISE output.
- **Expected:** The monophonic path should process multiple filter bands correctly, matching the polyphonic path's behaviour.

### PolyshapeFX -- Mode parameter range includes unregistered modes that silently fall back to Linear

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/WaveShapers.cpp:422-440
- **Observed:** The Mode parameter range is 0-33 (matching the ShapeFX::ShapeMode enum), but `PolyshapeFX::initShapers()` only registers 10 of the 34 slots. Modes 3 (Tanh), 6 (Saturate), 7 (Square), and 8 (SquareRoot) are valid enum values registered in ShapeFX but not in PolyshapeFX. Selecting these modes gives Linear passthrough with no warning. The shapers array is pre-filled with Linear instances for all slots, so any unregistered mode index silently degrades to passthrough. The metadata `Mode` parameter description says "populated dynamically from available shapers" but the range includes unavailable shapers.
- **Expected:** Either register the missing modes (Tanh, Saturate, Square, SquareRoot) in `PolyshapeFX::initShapers()`, or restrict the Mode parameter range/items to only the 10 actually registered modes so the UI does not present non-functional options.

### PolyphonicFilter -- StateVariablePeak and LadderFourPoleHP modes not wired in setMode()

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/FilterHelpers.cpp:54-117
- **Observed:** `FilterBank::setMode()` has no `case` for `FilterMode::StateVariablePeak` (index 11) or `FilterMode::LadderFourPoleHP` (index 16). Both fall through to `default: break;`, which means selecting either mode does not change the internal filter topology - the previously active filter type continues processing. The modes are defined in the `FilterMode` enum (FilterHelpers.h:49-69), have display coefficient entries in `getDisplayCoefficients()` (FilterHelpers.cpp:318,320), and are exposed in the scripting API via `FilterModeObject`. The frequency response graph updates (because `getDisplayCoefficients` handles them), but the actual audio processing uses the wrong filter.
- **Expected:** Add cases to `setMode()`: `StateVariablePeak` should map to `StateVariableFilterSubType` (needs a PEAK sub-type addition or map to `StateVariableEqSubType` which has a Peak mode), and `LadderFourPoleHP` should map to `LadderSubType` (needs an HP sub-type addition, as `LadderSubType` currently only defines `LP24`).

### PolyphonicFilter -- Quality parameter is vestigial (not consumed in render path)

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Filters.cpp:246, hi_core/hi_modules/effects/fx/FilterHelpers.h:495
- **Observed:** The `Quality` parameter is stored via `setRenderQuality()` into `FilterEffect::quality` and retrievable via `getSampleAmountForRenderQuality()`. However, neither `renderNextBlock()` nor `applyEffect()` reads this value. The mono path uses `BlockDivider<64>` (compile-time constant of 64 samples) for sub-block processing, and the poly path processes the entire voice buffer as one block. The `quality` member is never referenced in the signal path. The metadata description ("Internal render quality as power-of-two buffer size for modulation processing") suggests it should control the coefficient update interval, but it does not.
- **Expected:** Either connect `quality` to the sub-block size in `renderNextBlock()` (replacing the hardcoded `BlockDivider<64>`), or remove/deprecate the parameter in metadata.

### LegatoWithRetrigger -- onNoteOff Block 3 missing channel check

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_scripting/scripting/HardcodedScriptProcessor.h:242
- **Observed:** In `onNoteOff()`, Block 1 (line 226) checks both note number and channel (`Message.getNoteNumber() == lastNote && Message.getChannel() == lastChannel`) to identify the currently sounding note. However, Block 3 (line 242) only checks note number (`number == lastNote`) without a channel check. In multi-channel scenarios (e.g., MPE), a noteOff for the correct note number on a different channel would skip Block 1 (no event suppression or artificial noteOff) but enter Block 3 (triggering retrigger or clearing `lastNote`). This causes state desync: the active note's artificial event is not killed, but `lastNote` is either overwritten by a retrigger or set to -1.
- **Expected:** Block 3's condition should also check channel: `if(number == lastNote && channel == lastChannel)` to match Block 1's behavior.

### CurveEq -- setAttribute has no bounds check on band index

- **Type:** missing-validation
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/CurveEq.cpp (setInternalAttribute)
- **Observed:** `setAttribute(bandIndex * BandOffset + param, value)` does not validate the band index against the current band count. Addressing a non-existent band triggers an assertion failure in debug builds and undefined behaviour in release builds.
- **Expected:** Add a bounds check: if `bandIndex >= numBands`, return early or call `reportScriptError`.

### CurveEq -- Q parameter below 0.3 causes assertion failure

- **Type:** missing-validation
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/CurveEq.cpp (setInternalAttribute for Q)
- **Observed:** The Q parameter range is 0.3-8.0 in the UI, but scripted `setAttribute` calls can set values below 0.3, bypassing the UI range limit and triggering an assertion failure.
- **Expected:** Clamp Q to the valid range (0.3-8.0) in `setInternalAttribute` before applying.

### Arpeggiator -- Timer does not stop when DAW transport stops

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_scripting/scripting/hardcoded_modules/Arpeggiator.cpp:602-612
- **Observed:** The Arpeggiator timer continues running when the DAW transport is stopped. The `onTimer` function only checks if keys are held (`keys_are_held()`), with no DAW transport state check. `Synth.startTimer()` starts without verifying playback state. Users expect tempo-synced modules to stop with transport.
- **Expected:** Check DAW transport state in the timer logic. When transport stops, either pause the timer or stop the arpeggiator. A `TransportHandler` workaround exists but should not be necessary for a built-in tempo-synced module.

### Arpeggiator -- OctaveRange expansion can overflow int8 note numbers

- **Type:** missing-validation
- **Severity:** medium
- **Location:** hi_scripting/scripting/hardcoded_modules/Arpeggiator.cpp:632
- **Observed:** The octave expansion adds `(int8)(octaveSign * i * 12)` to held note numbers. The `NoteWithChannel` struct uses `int8` for `noteNumber` (max 127). When a high note (e.g., C7 = 96) is expanded by +3 octaves (+36), the result (132) overflows `int8`, wrapping to a negative value. No bounds check exists before the note is passed to `Synth.addNoteOn()`.
- **Expected:** Clamp expanded note numbers to the 0-127 range, or skip notes that would exceed the valid MIDI range.

### StreamingSampler -- KillThirdOldestNote repeat mode not implemented

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_sampler/sampler/ModulatorSampler.cpp:1446
- **Observed:** The `RepeatMode` enum defines `KillThirdOldestNote` (index 4, exposed as "Kill Third" in the UI), but `handleRetriggeredNote()` has no `case` for it. It falls through to `default: jassertfalse; break;`, meaning selecting this mode triggers an assertion in debug builds and does nothing in release builds. Only `KillSecondOldestNote` (Kill Duplicate) is implemented among the advanced repeat modes.
- **Expected:** Either implement KillThirdOldestNote (kill oldest voice when 3+ voices overlap on the same key) or remove it from the enum and UI to avoid user confusion.

### MatrixModulator -- Envelope sources do not update UI when no voice is active

- **Type:** ux-issue
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/MatrixModulator.cpp (render callback)
- **Observed:** Envelope-based sources in the MatrixModulator do not send UI updates (e.g. filter display) when no voice is active, because envelope render callbacks only fire during voice rendering. The filter display and any linked UI stay stale until a new note triggers. Acknowledged by Christoph Hart on the forum (tid 13409), planned fix for HISE 5.
- **Expected:** Envelope sources should send idle/default values to UI when voice count drops to zero.

### MatrixModulator -- extra_mod nodes non-functional in compiled FX plugins

- **Type:** silent-fail
- **Severity:** medium
- **Location:** scriptnode extra_mod integration path for monophonic FX context
- **Observed:** Parameters connected via `extra_mod` nodes and MatrixModulators work correctly in the HISE IDE but produce no modulation in compiled FX plugins (with AllowPolyphonic disabled). Direct parameter connections bypassing `extra_mod` work in both contexts. Christoph Hart noted this is an untested FX-context code path (tid 14354). Multiple users confirmed independently.
- **Expected:** `extra_mod` nodes should update `lastModValue` during compiled FX plugin rendering, matching IDE behaviour.

### HardcodedEnvelopeModulator -- MIDI CC events not forwarded into scriptnode envelope network

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting/scripting/HardcodedModules.cpp (handleHiseEvent / voiceStack)
- **Observed:** MIDI CC messages (e.g. sustain pedal CC64) are not forwarded into the scriptnode envelope modulator's network. A FlexAHDSR envelope outside scriptnode responds correctly to sustain pedal; the same envelope inside a HardcodedEnvelopeModulator does not. Reported by a trusted user (tid 13641).
- **Expected:** CC events should be forwarded through `voiceStack.handleHiseEvent()` so that sustain pedal and other CC-dependent behaviour works inside compiled envelope networks.

### GlobalTimeVariantModulator -- Crash on quit when used with global_mod scriptnode node

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/GlobalModulators.cpp / scriptnode global_mod node destructor
- **Observed:** A crash-on-quit was reported when combining a GlobalModulator with a `global_mod` scriptnode node. The issue was filed but not confirmed resolved at time of reporting (tid 13523). Likely a teardown order issue between the GlobalModulator and the scriptnode node.
- **Expected:** GlobalModulator and global_mod node destructors should handle disconnection gracefully during shutdown.

### GlobalVoiceStartModulator -- Returns wrong value when MIDI note number is remapped by a script

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/GlobalModulators.cpp (calculateVoiceStartValue)
- **Observed:** When a MIDI script blocks the incoming note and plays a different note number via `Synth.playNote()`, the global voice-start modulator looks up the value using the redirected note number rather than the original. The container pre-processes the event before the script remaps it, so the stored value corresponds to the original note number, but the consumer looks up by the new note number. This causes a mismatch - the wrong velocity/key value is returned. Reported by David Healey (tid 9158).
- **Expected:** The lookup should use a consistent note number. Either the container should store by event ID (not note number) or the consumer should look up using the original note number from the event.

### SimpleGain -- Pan/Balance modulation persistently broken across multiple versions

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/GainEffect.cpp (balance modulation path)
- **Observed:** Multiple reports spanning 2018-2024 confirm pan/balance modulation does not work reliably (tid 941, 1361, 6062). A related UI bug shows the bipolar pan table reporting 300L-100R instead of 100L-100R (tid 6062). The balance parameter range has also been reported as displaying 10000 instead of -100 to 100 (tid 10845).
- **Expected:** Balance modulation should work correctly and the parameter range should display the correct -100 to 100 range.

### Dynamics -- Limiter attack time change causes pops during playback

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Dynamics.cpp (limiterPending / setInternalAttribute)
- **Observed:** Christoph Hart confirmed that changing the limiter attack parameter during audio playback switches internal processing at a random point in the buffer and introduces a delay, causing clicks and pops (tid 2857, 3490). The attack time should not be automated.
- **Expected:** Limiter attack changes should use a crossfade or be deferred to a block boundary to prevent artefacts.

### SendFX -- Renaming a SendContainer breaks connected SendFX references

- **Type:** ux-issue
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/RouteFX.h (connect / container ID resolution)
- **Observed:** If a Send Container is renamed after Send Effects have been connected to it, the connection breaks and the SendFX dropdown shows 'no choices'. The fix is to remove and re-add the Send Container (tid 12586).
- **Expected:** Container references should update when the container is renamed, or use an ID-based reference rather than name-based.

### Analyser -- Changing BufferSize at runtime may break the display until project reload

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_dsp_library/node_api/helpers/RingBuffer.h (setRingBufferSize / DisplayBufferSource reconnection)
- **Observed:** Changing BufferSize at runtime caused the Analyser to stop displaying until the project was reopened (tid 5320). The ring buffer is resized at runtime but the display may not reconnect cleanly after a resize.
- **Expected:** The display should reconnect automatically after a runtime buffer resize without requiring a project reload.

### Analyser -- Audio mutes after recompiling script when DisplayBuffer references are reassigned

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Analyser.h (DisplayBufferSource / getDisplayBufferSource lifecycle)
- **Observed:** Audio was completely muted after the second script compile when using Synth.getDisplayBufferSource() and assigning display buffers to panel data properties. Restarting HISE restored audio (tid 12128). This may be a dangling reference issue when the ring buffer is reassigned on recompile.
- **Expected:** Display buffer references should survive script recompilation without affecting audio output.

### HardcodedMasterFX -- Peak meter shows identical values for both channels in compiled FX plugin

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/MasterEffectProcessor peak meter read path
- **Observed:** In a compiled effect plugin, peak meter tiles show the same value for left and right channels regardless of actual stereo content. Adding ENABLE_ALL_PEAK_METERS=1 to Extra Definitions resolves the issue (tid 8652).
- **Expected:** Peak meters should show independent left/right values by default in compiled plugins without requiring extra preprocessor definitions.

### ReleaseTrigger -- Editing the TimeTable curve editor crashes HISE

- **Type:** crash
- **Severity:** medium
- **Location:** hi_scripting/scripting/HardcodedScriptProcessor.h (ReleaseTrigger table access)
- **Observed:** Editing the TimeTable curve editor in the built-in ReleaseTrigger caused an instant crash. The fix was confirmed by the HISE author (tid 10531). Users on older builds should update.
- **Expected:** The table curve editor should be editable without crashing.

### ReleaseTrigger -- Time attenuation inconsistent when table curve is fully maxed

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting/scripting/HardcodedScriptProcessor.h (ReleaseTrigger table lookup / timeIndex normalisation)
- **Observed:** The time attenuation gave inconsistent velocity values when the table curve reached its maximum, appearing to report the value at position 0 instead of 0.5 at mid-hold times (tid 1956). A scripted implementation avoided the issue.
- **Expected:** The table lookup should return consistent interpolated values at all normalised time positions.

### MidiPlayer -- LoopEnabled toggle had no effect (loop always active)

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_dsp/modules/MidiPlayer.cpp (preprocessBuffer, LoopEnabled handling)
- **Observed:** Christoph Hart confirmed a regression where the LoopEnabled toggle had no effect and the player would always loop. Additionally, disabling the loop caused the first beat to be missed on each iteration (tid 7311). Workaround: use the sequence callback with `Math.ceil` on NumBars to normalise the loop boundary.
- **Expected:** The LoopEnabled parameter should correctly toggle between looping and one-shot playback modes.

### ScriptProcessor -- Components in secondary ScriptProcessor panels lose cross-script connections after restart

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting/scripting/ScriptProcessorModules (ScriptComponent save/load path)
- **Observed:** If a UI component in a secondary ScriptProcessor is inside a panel and is connected to a UI component on the Interface script via processorId/parameterId, the connection is silently lost on project reload. Removing and remaking the connection restores it temporarily. No fix was confirmed at the time of the report (tid 11430).
- **Expected:** Cross-script processorId/parameterId connections should persist across project reload.

### ScriptProcessor -- Timer inside a Synth Group may not fire correctly

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting/scripting/ScriptProcessorModules.cpp (processHiseEvent, timer channel matching)
- **Observed:** Timers in a ScriptProcessor nested inside a Synth Group have edge-case issues with visual updates. An expert confirmed the bug and recommended placing the timer ScriptProcessor outside the Synth Group or using setAttribute to update UI instead (tid 8290).
- **Expected:** Timer callbacks should fire correctly regardless of whether the ScriptProcessor is inside a Synth Group.

### ScriptFX -- Global cables non-functional for C++ nodes inside interpreted ScriptFX networks

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting scriptnode (GlobalCable dispatch, interpreted vs compiled network mode)
- **Observed:** When a C++ custom node is used inside a ScriptFX DSP network (interpreted mode), global cable connections from that node back to HISE do not function. The feature works correctly when the network is compiled and loaded into a HardcodedMasterFX module instead (tid 10657).
- **Expected:** Global cable connections should work for C++ nodes in both interpreted and compiled network modes.

### ScriptFX -- wet/dry template introduces attack artefact in polyphonic ScriptFX network

- **Type:** silent-fail
- **Severity:** medium
- **Location:** scriptnode wet/dry template (gain node resetValue)
- **Observed:** Using the built-in wet/dry template inside a polyphonic ScriptFX network produces an unwanted attack artefact. The fix is to adjust the resetValue parameter of the gain nodes inside the template (tid 12943).
- **Expected:** The wet/dry template should not introduce artefacts when used in a polyphonic network.

### SlotFX -- setBypassed() is not functional

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting/scripting/api/ScriptingApiObjects.cpp (SlotFX scripting class)
- **Observed:** Calling setBypassed() on a SlotFX reference throws a 'function not found' error. The function appears in the API browser but is not implemented on the SlotFX scripting object (tid 5947). Workaround: obtain a second reference to the same module using Synth.getEffect() and call setBypassed() on that.
- **Expected:** setBypassed() should be implemented on the SlotFX scripting object.

### SlotFX -- getParameterProperties() returns undefined

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_scripting/scripting/api/ScriptingApiObjects.cpp (~line 3937)
- **Observed:** Calling getParameterProperties() on a SlotFX reference returns undefined. This function only works with HardcodedMasterFX scriptnode effects, not with SlotFX itself (tid 13671).
- **Expected:** getParameterProperties() should either work on SlotFX or be excluded from the SlotFX scripting API.

## Low

### ArrayModulator -- Doxygen class comment is copy-pasted from ConstantModulator

- **Type:** ux-issue
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/ArrayModulator.h:37
- **Observed:** The Doxygen class comment reads "This modulator simply returns a constant value that can be used to change the gain or something else" which describes ConstantModulator, not ArrayModulator. The metadata description in ArrayModulator.cpp:39 is correct: "Creates a modulation signal from a slider pack array indexed by MIDI note number, allowing per-note modulation values."
- **Expected:** Update the class comment to match the metadata description or describe the slider-pack-indexed lookup behavior.

### Convolution -- ImpulseLength parameter is vestigial

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Convolution.cpp:122, 147-148
- **Observed:** The `ImpulseLength` parameter (index 3) is defined in the enum, serialised in `restoreFromValueTree`/`exportAsValueTree`, and registered in `createMetadata()` with description "Deprecated impulse length control". In `getAttribute()`, it returns a hardcoded `1.0f`. In `setInternalAttribute()`, it calls `setImpulse(sendNotificationAsync)` but stores no value - the reload uses the full buffer range from the AudioSampleProcessor. The parameter is fully vestigial. The metadata description correctly notes it as deprecated.
- **Expected:** Remove the parameter from the enum and metadata, or keep it for serialisation backwards compatibility with a clearer note that it is a no-op.

### Convolution -- Latency parameter stored but never consumed

- **Type:** vestigial
- **Severity:** medium
- **Location:** hi_core/hi_modules/effects/fx/Convolution.cpp:143-145; hi_dsp_library/dsp_basics/ConvolutionBase.cpp:610-621
- **Observed:** The `Latency` parameter (index 2) is stored as `latency = (int)newValue` and triggers `setImpulse(sendNotificationAsync)`. However, `reloadInternal()` derives the head block size from `lastBlockSize` (the audio buffer size), not from the `latency` member. The `latency` member is never read in the signal path or IR reload. Additionally: (1) the metadata range is 0.0-1.0 (uninitialised default), inconsistent with the description "in samples, which must be a power of two"; (2) `jassert(isPowerOfTwo(latency))` fires on the default value of 0 in debug builds since `isPowerOfTwo(0)` returns false. The description claims it controls convolution latency but the parameter has no effect on processing.
- **Expected:** Either wire the `latency` member into `reloadInternal()` as the head block size (replacing `lastBlockSize`), with a proper range and default, or remove the parameter if the automatic block-size-based head sizing is the intended behaviour.

### WaveSynth -- voicePitchValues incremented unconditionally when potentially null

- **Type:** code-smell
- **Severity:** low
- **Location:** hi_core/hi_modules/synthesisers/synths/WaveSynth.cpp:597
- **Observed:** In `calculateBlock()`, the single-oscillator pitch-modulated path (line 585-602) is entered when the outer condition `(voicePitchValues == nullptr && secondPitchValues == nullptr)` is false. Inside this block, line 591 checks `if (voicePitchValues != nullptr)` before dereferencing, but line 597 increments `voicePitchValues++` unconditionally. If `voicePitchValues` is null (possible when only `secondPitchValues` is non-null), this increments a null pointer. In practice this is unreachable because `enableSecondOsc` is false in this branch and having a populated Osc2PitchChain with osc2 disabled is not a realistic configuration.
- **Expected:** Guard the increment with `if (voicePitchValues != nullptr)` to match the dual-oscillator path (line 579-580), or restructure the outer condition to check `voicePitchValues` independently.

### NoiseGrainPlayer -- setIncludeMonophonicValuesInVoiceRendering called twice on same chain

- **Type:** code-smell
- **Severity:** low
- **Location:** hi_core/hi_modules/effects/fx/NoiseGrainPlayer.cpp:157-158
- **Observed:** The constructor calls `modChains[InternalChains::PositionBipolar].setIncludeMonophonicValuesInVoiceRendering(true)` twice (lines 157 and 158). The second call appears to be a copy-paste error - line 158 should likely target `PositionChain` (index 0) instead of `PositionBipolar` (index 1) again. As a result, `PositionChain` never has monophonic values included in voice rendering, which may affect modulation behavior when monophonic modulators are added to the gain chain.
- **Expected:** Line 158 should be `modChains[InternalChains::PositionChain].setIncludeMonophonicValuesInVoiceRendering(true);`

### NoiseGrainPlayer -- EditorStates enum has duplicate values

- **Type:** code-smell
- **Severity:** low
- **Location:** hi_core/hi_modules/effects/fx/NoiseGrainPlayer.h:68-69
- **Observed:** `PositionChainShown` and `PositionBipolarShown` are both set to `Processor::numEditorStates`. The second entry should increment from the first. This means both editor states map to the same bit, so toggling one toggles the other.
- **Expected:** `PositionBipolarShown` should be `Processor::numEditorStates + 1` (or simply follow `PositionChainShown` without explicit assignment).

### NoiseGrainPlayer -- Header class comment is copy-pasted from StereoEffect

- **Type:** ux-issue
- **Severity:** low
- **Location:** hi_core/hi_modules/effects/fx/NoiseGrainPlayer.h:37
- **Observed:** The Doxygen class comment reads "A simple stereo panner which can be modulated using all types of Modulators" which describes StereoEffect, not NoiseGrainPlayer. The `JUCE_DECLARE_WEAK_REFERENCEABLE(StereoEffect)` macro at line 233 also references StereoEffect instead of NoiseGrainPlayer, confirming the class was scaffolded from StereoEffect.
- **Expected:** Update the class comment to describe the granular noise player, and fix the weak-referenceable macro to use `NoiseGrainPlayer`.

### Transposer -- Metadata description misleadingly says "note-on events" only

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_core/hi_modules/midi_processor/mps/Transposer.cpp:42, 45
- **Observed:** Both the module description ("Transposes incoming MIDI note-on events by a fixed number of semitones") and the TransposeAmount parameter description ("Semitone offset applied to incoming note-on events") say "note-on events". While technically accurate at the processor level (only `processHiseEvent` touches note-ons), the EventIdHandler automatically propagates the transpose amount to matching note-off events (HiseEventBuffer.cpp:1003,1019). From a user perspective, both note-on and note-off are transposed. The current wording could mislead users into thinking note-offs are not handled.
- **Expected:** Change "note-on events" to "MIDI notes" in both the module description and the parameter description to reflect the effective end-to-end behavior.

### ChannelFilter -- MPE mode initial state does not allow channel 1

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_scripting/scripting/HardcodedScriptProcessor.h:642-644
- **Observed:** In `onInit()`, the `mpeRange` bitmask is set to `mpeRange.setRange(1, 15, true)` which enables bits 1-15 (channels 2-16) but does not set bit 0 (channel 1, the MPE master channel). The `onControl()` handler (line 723) always force-enables bit 0 with `mpeRange.setBit(0, true)` after any MPE parameter change. This means on initial load (before the user changes any MPE parameter), if MPE mode is enabled globally, channel 1 events would be filtered out. After any MPE parameter change, channel 1 is correctly allowed. The MPE spec designates channel 1 as the manager channel which should always pass through.
- **Expected:** Add `mpeRange.setBit(0, true)` after line 644 in `onInit()` to match the behavior in `onControl()`, ensuring channel 1 is allowed from initial load.

### KeyNumber -- Doxygen class comment is copy-pasted from RandomModulator

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/KeyModulator.h:37
- **Observed:** The Doxygen comment reads "A constant Modulator which calculates a random value at the voice start" which describes RandomModulator, not KeyModulator. The class actually maps MIDI note numbers to modulation values through a lookup table.
- **Expected:** Update the Doxygen comment to describe the actual behavior, e.g., "A VoiceStartModulator which maps MIDI note numbers to modulation values through a lookup table."

### ReleaseTrigger -- Time parameter uses HiSlider::Time mode but operates in seconds

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_scripting/scripting/HardcodedScriptProcessor.cpp:78
- **Observed:** The Time parameter metadata uses `HiSlider::Time` mode which displays with "ms" suffix (per MacroControlledComponents.h:751). However, the parameter range is 0-20 with step 0.1, and the code in `onNoteOff()` (HardcodedScriptProcessor.h:407-410) divides the elapsed time from `Engine.getUptime()` (which returns seconds) directly by `timeKnob->getValue()`. The parameter value is treated as seconds in the signal path, but displayed as milliseconds in the UI. A Time value of 5 would display as "5 ms" but actually represent a 5-second attenuation window.
- **Expected:** Either change the slider mode from `HiSlider::Time` to `HiSlider::Linear` with a custom suffix "s", or multiply the parameter value by 0.001 in the code to convert from the displayed milliseconds to seconds. The former is simpler and matches actual usage.

### Constant -- Metadata description says "1.0" but GainMode returns 0.0

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/ConstantModulator.cpp:39; ConstantModulator.h:79
- **Observed:** The `createMetadata()` description reads "Creates a constant modulation signal (1.0)..." but `calculateVoiceStartValue()` returns `0.0f` in GainMode (the most common use case). The code comment says "Returns 0.0f and let the intensity do its job." The effective output with default intensity (1.0) is indeed unity gain, so the end result matches the description, but the raw modulator output is 0.0, not 1.0.
- **Expected:** Update the metadata description to say "Creates a constant modulation signal that can be controlled via setIntensity()" or similar wording that does not claim a specific numeric output.

### Random -- Vestigial currentValue member variable

- **Type:** vestigial
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/RandomModulator.h:74
- **Observed:** The `currentValue` member is declared as `volatile float` in the class but is never read or written in the implementation (constructor, calculateVoiceStartValue, setInternalAttribute, getAttribute). It appears to be a leftover from an earlier implementation where the last generated value was cached.
- **Expected:** Remove the unused member variable.

### Random -- Header comment claims 7-bit quantisation that does not exist

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/RandomModulator.h:42
- **Observed:** The Doxygen class comment states "the values are limited to 7bit for MIDI feeling" when the table is used. The actual implementation uses `getInterpolatedValue()` on a 512-entry table with full float precision linear interpolation. No 7-bit quantisation exists in the code.
- **Expected:** Update the comment to reflect the actual behavior (512-point interpolated lookup with full float precision).

### MidiController -- DefaultValue parameter is broken due to uint8 cast on 0-1 float

- **Type:** silent-fail
- **Severity:** high
- **Location:** hi_core/hi_modules/modulators/mods/ControlModulator.cpp:196
- **Observed:** In `setInternalAttribute(DefaultValue)`, the code synthesizes a fake CC event: `handleHiseEvent(HiseEvent(HiseEvent::Type::Controller, (uint8)controllerNumber, (uint8)defaultValue, 1))`. The `defaultValue` member is a float in range 0.0-1.0 (NormalizedPercentage slider mode). Casting a 0.0-1.0 float to `uint8` always produces 0 (for values < 1.0) or 1 (for exactly 1.0). This value is then divided by 127.0 in `handleHiseEvent`, yielding either 0.0 or 0.0079 as the effective modulation value. The DefaultValue slider appears functional in the UI but has no meaningful effect for any setting other than 0%.
- **Expected:** The cast should scale the 0-1 value to 0-127 before truncating: `(uint8)(defaultValue * 127.0f)`. Alternatively, bypass the HiseEvent synthesis and set `inputValue`/`targetValue` directly from `defaultValue`.

### MidiController -- Preliminary JSON has aftertouch and pitch wheel CC numbers swapped

- **Type:** inconsistency
- **Severity:** high
- **Location:** module_enrichment/preliminary/MidiController.json (ControllerNumber description)
- **Observed:** The preliminary JSON ControllerNumber description says "128 for aftertouch, 129 for pitch wheel". The actual C++ constants are `HiseEvent::PitchWheelCCNumber = 128` and `HiseEvent::AfterTouchCCNumber = 129` (defined in hi_tools/hi_tools/HiseEventBuffer.h:87-88). The metadata description in `createMetadata()` also has them swapped, stating "128 for aftertouch, 129 for pitch wheel".
- **Expected:** Both the preliminary JSON and the C++ metadata description should read "128 for pitch wheel, 129 for aftertouch" to match the actual constant definitions.

### FlexAHDSR -- Preliminary JSON incorrectly lists VoiceStartModulator constrainer for SustainLevelModulation

- **Type:** inconsistency
- **Severity:** low
- **Location:** module_enrichment/preliminary/FlexAHDSR.json (SustainLevelModulation chain entry)
- **Observed:** The preliminary JSON lists `"constrainer": "VoiceStartModulator"` for the SustainLevelModulation chain. However, the C++ metadata registration at FlexAhdsrEnvelope.cpp:113-116 does NOT call `.withConstrainer<VoiceStartModulatorFactoryType::Constrainer>()` for this chain (unlike all other four chains). The chain is created with `ModulationType::Normal` (not `VoiceStartOnly`) and `calculateBlock()` has a dedicated per-sample code path for time-variant sustain modulation. The chain intentionally accepts all modulator types.
- **Expected:** Remove or correct the constrainer field for SustainLevelModulation in the preliminary JSON and downstream documentation. The chain should be documented as accepting both voice-start and time-variant modulators.

### EventDataModulator -- SlotIndex parameter allows index 16, which silently aliases to slot 0

- **Type:** silent-fail
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/EventDataModulator.h:91
- **Observed:** `setInternalAttribute` uses `jlimit<uint8>(0, AdditionalEventStorage::NumDataSlots, ...)` where `NumDataSlots = 16`, allowing index 16. The slider range also goes to 16.0. However, `AdditionalEventStorage::getValue()` masks the index with `(NumDataSlots - 1)` = 15, so index 16 (0b10000) silently maps to slot 0. Users setting slot 16 unknowingly read slot 0.
- **Expected:** The `jlimit` upper bound should be `NumDataSlots - 1` (15), and the slider range max should be 15.0. The same issue exists in EventDataEnvelope at the same file line 239.

### EventDataEnvelope -- SmoothingTime slider tooltip is copy-pasted from DefaultValue

- **Type:** ux-issue
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/EventDataModulator.cpp:130
- **Observed:** The `smoothingSlider->setTooltip()` text reads "The value if the event data hasn't been written", which is the DefaultValue tooltip. The SmoothingTime tooltip should describe the smoothing time parameter.
- **Expected:** Change the tooltip to something like "Smoothing time for value changes in milliseconds" to match the parameter's actual function.

### EventDataModulator -- Doxygen class comment is copy-pasted from RandomModulator

- **Type:** inconsistency
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/EventDataModulator.h:38-43
- **Observed:** The class Doxygen comment reads "A constant Modulator which calculates a random value at the voice start" and mentions a "look up table to massage the outcome." EventDataModulator has no randomness and no table. This is a copy-paste artifact from RandomModulator.
- **Expected:** Update the comment to describe actual behavior: reads a value from an event data slot written via the GlobalRoutingManager.

### EventDataModulator -- Cached additionalEventStorage pointer is unused

- **Type:** code-smell
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/EventDataModulator.h:107, EventDataModulator.cpp:188-206
- **Observed:** The constructor caches `additionalEventStorage` from the GlobalRoutingManager (line 85 of .cpp). However, `calculateVoiceStartValue()` ignores this cached pointer and instead re-fetches the GlobalRoutingManager from MainController on every call (lines 190-193). By contrast, EventDataEnvelope correctly uses its cached pointer. The member variable is dead code in EventDataModulator.
- **Expected:** Either use the cached pointer in `calculateVoiceStartValue()` (consistent with EventDataEnvelope) or remove the member variable.

### MPEModulator -- Glide gesture normalization is asymmetric (upward range truncated)

- **Type:** inconsistency
- **Severity:** medium
- **Location:** hi_core/hi_modules/modulators/mods/MPEModulators.cpp:519-522
- **Observed:** The Glide gesture normalization formula is `((pitchWheelValue - 8192) / 2048) * 0.5 + 0.5`. The divisor is 2048 instead of 8192. This maps the 14-bit pitch bend range (0-16383, center 8192) to approximately -2.0 to +2.0 before the 0.5 scale and offset, resulting in a range of approximately -0.5 to +1.5. After `jlimit(0, 1, midiValue)`, downward pitch bend saturates at 0.0 at about 25% of full range, and upward pitch bend saturates at 1.0 at about 25% of full range. Only the central ~50% of pitch bend travel produces useful modulation variation. If the divisor were 8192, the full pitch bend range would map symmetrically to 0.0-1.0.
- **Expected:** Use 8192 as the divisor for symmetric full-range mapping: `((pitchWheelValue - 8192.0f) / 8192.0f) * 0.5f + 0.5f`. Alternatively, if the reduced range is intentional for finer control, document this behavior.

### MPEModulator -- SmoothedIntensity parameter name is misleading

- **Type:** ux-issue
- **Severity:** low
- **Location:** hi_core/hi_modules/modulators/mods/MPEModulators.cpp:209-221
- **Observed:** The parameter named "SmoothedIntensity" with description "The intensity of the modulation with smoothing applied" actually sets the modulator's overall intensity via `setIntensity()`. It has no relationship to the smoothing process. Users may expect it controls how much smoothing is applied or blends between smoothed and raw values. It is functionally identical to a standard "Intensity" or "Depth" parameter.
- **Expected:** Consider renaming to "Intensity" or "Depth" in the metadata, or update the description to clarify it controls overall modulation amplitude, not smoothing behavior.
