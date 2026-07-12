# Synth -- Class Analysis

## Brief
Global namespace for MIDI generation, voice control, module tree access, and timer callbacks on the parent synth.

## Purpose
The `Synth` object provides script-level access to the parent `ModulatorSynth` that hosts the script processor. It is a global namespace (not a user-created object) automatically available in every script callback. Its primary responsibilities are: generating artificial MIDI events (notes, controllers, pitch/volume fades), navigating the module tree to obtain handles to child processors (effects, modulators, samplers, MIDI processors), managing a sample-accurate timer system for periodic callbacks, and querying keyboard/voice state. All module tree operations are scoped to the parent synth's subtree (with a few global-search exceptions), and most `get*()` methods are restricted to the `onInit` callback to prevent audio-thread allocations.

## Details

### Module Tree Navigation

The `get*()` methods use two distinct search strategies (see individual method entries for details):

**Owner-rooted search (subtree only):** Most methods -- `getModulator`, `getEffect`, `getMidiProcessor`, `getChildSynth`, `getSampler`, `getSlotFX`, `getAudioSampleProcessor`, `getTableProcessor`, `getSliderPackProcessor`, `getDisplayBufferSource`, `getIdList` -- iterate only the subtree rooted at the parent synth using `Processor::Iterator<T>(owner)`. These are restricted to `onInit` via `objectsCanBeCreated()`.

**Global-rooted search (entire tree):** `getMidiPlayer`, `getRoutingMatrix`, `getWavetableController`, and `getAllModulators` search from `getMainSynthChain()` and do NOT enforce the `onInit` restriction.

`getMidiProcessor` has a self-exclusion check -- you cannot get a reference to the script processor that owns this Synth object.

`getSlotFX` performs a dual search: first for `HotswappableProcessor` (traditional SlotFX), then falls back to `DspNetwork::Holder` (scriptnode-based slot).

### MIDI Event Generation

All script-generated events are flagged as **artificial** (`setArtificial()`). Three note-on methods provide different levels of control (see `playNote`, `addNoteOn`, `playNoteWithStartOffset` for full details):

| Method | Channel | Timestamp | Start Offset |
|--------|---------|-----------|--------------|
| `playNote` | Fixed ch 1 | 0 | 0 |
| `addNoteOn` | Explicit | Explicit | 0 |
| `playNoteWithStartOffset` | Explicit | 0 | Explicit |

Note-off methods: `noteOffByEventId` (immediate), `noteOffDelayedByEventId` (with sample delay), `addNoteOff` (low-level with channel/timestamp). The deprecated `noteOff(noteNumber)` still works but reports an error.

The `attachNote` system links an artificial note to a real note so stopping the original automatically stops the artificial one. Requires `setFixNoteOnAfterNoteOff(true)` first.

`addVolumeFade` with `targetVolume == -100` triggers a "fade to silence and kill" -- it creates both the volume fade and an automatic note-off at the end of the fade time.

### Timer System

Two operating modes depending on `deferCallbacks` (see `startTimer`, `stopTimer`, `deferCallbacks` for full details):

| Mode | Thread | Accuracy | Slots |
|------|--------|----------|-------|
| Non-deferred (default) | Audio thread | Sample-accurate (HISE_EVENT_RASTER) | 4 per synth |
| Deferred | Message thread | Millisecond (JUCE Timer) | Unlimited |

Minimum interval: 4ms (0.004 seconds). Timer events in non-deferred mode are injected as `HiseEvent::TimerEvent` into the event buffer.

### Controller Events

`sendController` and `addController` handle three event types through the same interface (see individual method entries for artificial flag differences):

| CC Number | Event Type | Value Range |
|-----------|-----------|-------------|
| 0-127 | Standard CC | 0-127 |
| 128 | Pitch Bend | 0-16383 |
| 129 | Aftertouch | 0-127 |

`sendController` inherits timestamp from current event and does NOT set the artificial flag. `addController` has explicit channel/timestamp and DOES set the artificial flag.

### Modulator Chain Access

The `chainId` parameter uses the C++ `ModulatorSynth::InternalChains` enum values directly: `1 = GainModulation`, `2 = PitchModulation`. See `addModulator`, `getModulatorIndex`, and `setModulatorAttribute` for full details.

`setModulatorAttribute` supports special attribute indices: `-12` for Intensity and `-13` for Bypassed. For pitch chain intensity, the value is converted from semitones to ratio using `pow(2, value/12.0)`.

### Attribute System

`getAttribute`/`setAttribute` operate on the parent synth's parameter indices. See those method entries for full parameter tables. Standard `ModulatorSynth` indices: 0=Gain (0.0-1.0), 1=Balance (-100 to 100), 2=VoiceLimit, 3=KillFadeTime. Subclasses extend this enum with additional parameters.

### Clock Speed Values

`setClockSpeed` accepts musical division values. See the method entry for the full value table and error handling.

### Host Processor Context

The Synth object exists in every script processor type (JavascriptMidiProcessor, Script Voice Start Modulator, Script Time Variant Modulator, Script Envelope Modulator, Script FX, Script Polyphonic FX, Script Synthesiser). However, MIDI-generating methods (`playNote`, `addNoteOn`, `addVolumeFade`, etc.) require a `ScriptBaseMidiProcessor` host -- they will fail with "Only valid in MidiProcessors" when called from modulator or effect scripts.

### Backwards Compatibility

- `noteOff(noteNumber)` -- deprecated, use `noteOffByEventId`
- `sendControllerToChildSynths` -- identical to `sendController`, kept for compatibility
- `setUseUniformVoiceHandler` -- fully deprecated, throws error immediately
- `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` (default on) -- subtracts one audio block from timestamps on the audio thread

## obtainedVia
Automatically available as the global `Synth` namespace in every script processor. Not user-created.

## minimalObjectToken


## Constants
None. Synth is instantiated with `ApiClass(0)` and has no `addConstant()` calls.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var mod = Synth.getModulator("LFO1");` in `onNoteOn` | `var mod = Synth.getModulator("LFO1");` in `onInit` | Most get*() methods can only be called in onInit. Store references as top-level variables. |
| `Synth.noteOff(60);` | `Synth.noteOffByEventId(eventId);` | noteOff by note number is deprecated and unreliable with multiple voices on the same note. Use event IDs. |
| `Synth.playNote(60, 0);` | `Synth.playNote(60, 1);` | playNote rejects velocity 0. Use noteOffByEventId to stop notes. |
| `Synth.addModulator(0, "LFO", "MyLFO");` | `Synth.addModulator(2, "LFOModulator", "MyLFO");` | chainId uses the C++ enum values: 1=Gain, 2=Pitch. Using 0 is invalid for modulator chains. |
| `Synth.setMacroControl(0, 64.0);` | `Synth.setMacroControl(1, 64.0);` | macroIndex is 1-based (1-8), not 0-based. Only works when parent synth is a ModulatorSynthChain. |

## codeExample
```javascript
// Synth is a global namespace -- no creation needed.
// Store module references in onInit:
const var lfo = Synth.getModulator("LFO1");
const var fx = Synth.getEffect("Delay1");
```

## Alternatives
- `Engine` -- global utilities and object creation unrelated to a specific synth
- `ChildSynth` -- handle to a specific child sound generator retrieved via `Synth.getChildSynth()`

## Related Preprocessors
`USE_BACKEND` (ModuleDiagnoser compile-time checks), `ENABLE_SCRIPTING_SAFE_CHECKS` (extra validation), `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` (timestamp adjustment), `HISE_EVENT_RASTER` (timer event sample alignment).

## Diagrams

### synth-module-tree-search
- **Brief:** Module Tree Search Strategies
- **Type:** topology
- **Description:** Shows two search strategies used by Synth's get*() methods. Owner-rooted search (used by getModulator, getEffect, getMidiProcessor, getChildSynth, getSampler, getSlotFX, etc.) iterates only the subtree rooted at the parent synth. Global-rooted search (used by getMidiPlayer, getRoutingMatrix, getWavetableController, getAllModulators) starts from the MainSynthChain and searches the entire module tree. Both converge on returning typed script wrapper objects.

### synth-midi-event-flow
- **Brief:** MIDI Event Generation Flow
- **Type:** topology
- **Description:** Shows how the three note-on methods (playNote, addNoteOn, playNoteWithStartOffset) all funnel into internalAddNoteOn, which creates a HiseEvent with artificial flag, registers it with the EventHandler and Message object, and inserts it into the MIDI buffer. The note-off path shows noteOffByEventId and noteOffDelayedByEventId using the EventHandler to pop the matching note-on. addVolumeFade with targetVolume=-100 branches into both volume fade creation and automatic note-off generation.

## Diagnostic Ideas
Reviewed: Yes
Count: 2
- Synth.attachNote -- timeline dependency: requires setFixNoteOnAfterNoteOff before use (logged)
- Synth.setClockSpeed -- value check: only accepts 0, 1, 2, 4, 8, 16, 32 (logged)
