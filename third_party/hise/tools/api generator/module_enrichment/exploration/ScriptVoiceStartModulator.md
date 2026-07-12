# Script Voice Start Modulator - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h`, `hi_scripting/scripting/ScriptProcessorModules.cpp`
**Base class:** `VoiceStartModulator` + `JavascriptProcessor` + `ProcessorWithScriptingContent`

## Signal Path

The module operates in the voice-start modulator slot of a sound generator's modulation chain. When a note-on triggers a new voice, the `startVoice(voiceIndex)` method is called. This executes the `onVoiceStart` callback with the voice index as a parameter. The return value of the callback becomes the modulation value for that voice (0.0 to 1.0).

MIDI events are received via `handleHiseEvent()` which routes:
- Note-off events -> `onVoiceStop` callback (with voiceIndex parameter set to 0)
- Controller events -> `onController` callback

The `onControl` callback is triggered by UI component value changes, separate from the MIDI event path.

## Gap Answers

### callback-list: What are all the callbacks available?

Five callbacks, defined in `JavascriptVoiceStartModulator::Callback` enum:

1. **`onInit`** - No parameters. Runs once when the script compiles. Used to declare variables and create UI components.
2. **`onVoiceStart(voiceIndex)`** - One parameter: `voiceIndex` (int). Fires when a new voice starts. **Must return a float value** (0.0 to 1.0) that becomes the per-voice modulation value.
3. **`onVoiceStop(voiceIndex)`** - One parameter: `voiceIndex` (int). Fires on note-off events. Note: in the current implementation, the voiceIndex is always set to 0 (hardcoded), not the actual stopping voice index.
4. **`onController`** - No parameters. Fires on controller change events. Access the event via the `Message` object.
5. **`onControl(number, value)`** - Two parameters: `number` (the UI component reference) and `value` (the new value). Fires when a script-defined UI component's value changes.

### return-value-semantics: What should onVoiceStart return?

The `onVoiceStart` callback's return value is cast to float and stored as `unsavedValue`. This value is then passed to `VoiceStartModulator::startVoice(voiceIndex)` which stores it in the per-voice modulation value array.

The expected range is 0.0 to 1.0 in gain mode, where:
- 1.0 = no modulation (full pass-through)
- 0.0 = full attenuation (silence)

Before the callback executes, `synthObject->setVoiceGainValue(voiceIndex, 1.0f)` and `synthObject->setVoicePitchValue(voiceIndex, 1.0f)` are called to reset the voice state.

### voice-stop-callback: What does onVoiceStop do?

The `onVoiceStop` callback fires when a note-off event is received (checked via `m.isNoteOff()` in `handleHiseEvent()`). The voiceIndex parameter is hardcoded to 0 in the current implementation (`scriptEngine->setCallbackParameter(onVoiceStop, 0, 0)`), which means it does not accurately reflect which voice is stopping. This callback is primarily useful for bookkeeping or cleanup tasks that don't need the actual voice index, such as decrementing a note counter or updating a display.

### api-objects-available: What API objects are registered?

From `registerApiClasses()`:
- **`Message`** - Access the current MIDI event
- **`Engine`** - Global engine functions
- **`Synth`** - Control the parent synthesiser
- **`Console`** - Debug output
- **`Content`** - UI component creation (registered as native object)
- **`ModulatorApi`** - Modulator-specific functions

Note: compared to ScriptProcessor, this module does NOT have access to: `Settings`, `FileSystem`, `Threads`, `Date`, `Server`, `Colours`, `Sampler`, `Libraries`, or `Buffer`. The API surface is more limited.

### on-control-parameters: What parameters does onControl receive?

Same as ScriptProcessor: two parameters:
1. `number` - Reference to the ScriptComponent that changed
2. `value` - The new value of the component

### dynamic-parameters: How are parameters defined?

Same as ScriptProcessor: parameters are defined dynamically through UI components created in the `onInit` callback using the `Content` API. Each component becomes a parameter.

## Processing Chain Detail

1. **Event Reception** (`handleHiseEvent`) - Receives HiseEvent, routes to appropriate callback. CPU: negligible.
2. **Note Counter Update** (`synthObject->handleNoteCounter`) - Tracks active note count. CPU: negligible.
3. **Voice Start** (`startVoice`) - Executes onVoiceStart callback when a new voice begins. Returns modulation value. CPU: depends on user script.
4. **Voice Stop** (`handleHiseEvent` on note-off) - Executes onVoiceStop callback. CPU: depends on user script.
5. **Controller Handling** (`handleHiseEvent` on controller) - Executes onController callback. CPU: depends on user script.

## Conditional Behavior

### Empty Callback Optimization
Each callback checks `isSnippetEmpty()` before execution. If empty, the script engine is not invoked.

### Voice Start Reset
Before executing onVoiceStart, the voice gain and pitch values are reset to 1.0. This ensures a clean slate for each voice calculation.

## CPU Assessment

- **Framework overhead:** negligible
- **Actual CPU cost:** depends on user script, but executes only at note-on (not per-sample), so typically low
- **Baseline tier:** negligible (the framework itself)
- **Per-voice:** yes, onVoiceStart runs once per voice start

## UI Components

Uses `ScriptingEditor` as its editor component (same as ScriptProcessor).

## Notes

The voiceIndex parameter in `onVoiceStop` is hardcoded to 0, which is a known limitation. The actual voice index is not passed to the callback.

The module has a more limited API surface than ScriptProcessor - it lacks access to Server, FileSystem, Threads, Date, Settings, Colours, Libraries, Buffer, and Sampler objects. This is appropriate for its role as a lightweight modulation calculator.

Unlike ScriptProcessor, there is no deferred mode or timer mechanism available. All callbacks execute on the audio thread.
