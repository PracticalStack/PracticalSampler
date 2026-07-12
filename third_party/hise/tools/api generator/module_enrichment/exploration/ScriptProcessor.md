# Script Processor - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h`, `hi_scripting/scripting/ScriptProcessorModules.cpp`
**Base class:** `ScriptBaseMidiProcessor` (which extends `MidiProcessor` + `ProcessorWithScriptingContent`), also extends `JavascriptProcessor` and `Timer`

## Signal Path

MIDI events arrive via `processHiseEvent()`. The method routes each event to the appropriate callback based on event type:

- NoteOn -> `onNoteOn` callback
- NoteOff -> `onNoteOff` callback
- Controller, PitchBend, Aftertouch, ProgramChange -> `onController` callback
- TimerEvent -> `onTimer` callback (only if the timer event's channel matches this processor's chain index)

The `onControl` callback is triggered separately by UI component value changes, not by MIDI events.

The `onInit` callback runs once at compile time (when the script is compiled).

In deferred mode, all MIDI callbacks are forwarded to the message thread via an async queue rather than executing on the audio thread.

## Gap Answers

### callback-list: What are all the callbacks available in the ScriptProcessor?

Six callbacks, defined in `ScriptBaseMidiProcessor::Callback` enum:

1. **`onInit`** - No parameters. Runs once when the script compiles. Used to declare variables, create UI components, set up module references.
2. **`onNoteOn`** - No parameters. Fires on every MIDI note-on event. Access the event via the `Message` object.
3. **`onNoteOff`** - No parameters. Fires on every MIDI note-off event. Access the event via the `Message` object.
4. **`onController`** - No parameters. Fires on controller changes, pitch bend, aftertouch, and program change events. Access the event via the `Message` object.
5. **`onTimer`** - No parameters. Fires at the interval set by `Synth.startTimer()`. Timer events are sample-accurate when not deferred.
6. **`onControl(number, value)`** - Two parameters: `number` (the UI component reference) and `value` (the new value). Fires when a script-defined UI component's value changes.

### callback-execution-context: Which callbacks run on the audio thread?

By default, all MIDI callbacks (onNoteOn, onNoteOff, onController, onTimer) run on the **audio thread** via `processHiseEvent()`. This means they are real-time safe constraints apply.

When `Synth.deferCallbacks(true)` is called, MIDI events are queued via `DeferredExecutioner` and processed on the **message thread** via `JavascriptThreadPool`. In deferred mode, the timer uses a JUCE `Timer` instead of sample-accurate timer events.

The `onControl` callback runs on the **scripting thread** (via `JavascriptThreadPool`) when triggered from the message thread, or directly on the audio thread if triggered from within an audio-thread callback. The `onInit` callback always runs on the scripting thread during compilation.

### api-objects-available: What API objects are registered?

From `registerApiClasses()`:
- **`Message`** - Access and modify the current MIDI event (note number, velocity, channel, controller number/value, etc.)
- **`Engine`** - Global engine functions (sample rate, host info, module access, etc.)
- **`Synth`** - Control the parent synth (start/stop timer, add/play notes, get/set module parameters, voice management)
- **`Sampler`** - Sampler-specific functions (only functional when parent is a ModulatorSampler)
- **`Content`** - UI component creation and management (registered as native object)
- **`Console`** - Debug output
- **`Settings`** - Project settings access
- **`FileSystem`** - File I/O
- **`Threads`** - Threading utilities
- **`Date`** - Date/time functions
- **`Server`** - HTTP server/client functionality
- **`Colours`** - Colour constants
- **`Libraries`** - DSP library loader
- **`Buffer`** - Audio buffer factory

### timer-mechanism: How does the onTimer callback work?

The timer is started with `Synth.startTimer(seconds)` and stopped with `Synth.stopTimer()`.

In non-deferred mode: The timer generates `HiseEvent::Type::TimerEvent` events that are injected into the MIDI event stream. These are sample-accurate - the timestamp in the event determines the exact sample position. The `runTimerCallback()` method executes the onTimer snippet. The timer event is channel-matched to the processor's chain index so multiple ScriptProcessors can have independent timers.

In deferred mode: The JUCE `Timer` class is used instead, calling `timerCallback()` on the message thread. This is not sample-accurate.

### on-control-parameters: What parameters does onControl receive?

The `onControl` callback receives two parameters:
1. `number` - A reference to the ScriptComponent that changed (e.g., a knob, button, combobox)
2. `value` - The new value of the component (as a `var` - can be numeric, string, etc.)

The callback is only triggered for components that are not connected to a processor parameter, macro control, global cable, or custom automation. If a component has a `processorId` and `parameterId` property set, the value is forwarded directly to the target processor without triggering onControl.

### front-interface: What does the 'front' mode do?

When a ScriptProcessor is set as "front" (via `Content.makeFrontInterface(width, height)`), it becomes the main interface script processor. There can only be one front interface per plugin. The front interface:
- Renders its Content as the plugin's main UI
- Has its Content update dispatcher active in the frontend build
- Is discoverable via `getFirstInterfaceScriptProcessor()`

Non-front ScriptProcessors still process MIDI and can have Content, but their UI is not shown as the plugin interface.

### midi-event-types: What MIDI event types trigger which callbacks?

From `runScriptCallbacks()`:
- `HiseEvent::Type::NoteOn` -> `onNoteOn`
- `HiseEvent::Type::NoteOff` -> `onNoteOff`
- `HiseEvent::Type::Controller` -> `onController`
- `HiseEvent::Type::PitchBend` -> `onController`
- `HiseEvent::Type::Aftertouch` -> `onController`
- `HiseEvent::Type::ProgramChange` -> `onController`
- `HiseEvent::Type::TimerEvent` -> `onTimer` (only if channel matches chain index)
- `HiseEvent::Type::AllNotesOff` -> handled internally (note counter reset), no user callback

Note: CC#64 (sustain pedal) is handled specially - `synthObject->setSustainPedal()` is called before the onController callback.

### dynamic-parameters: How are parameters defined?

Parameters are defined dynamically through UI components created in the `onInit` callback using the `Content` API (e.g., `Content.addKnob()`, `Content.addButton()`, `Content.addComboBox()`). Each component becomes a parameter. The module's `metadataType` is "dynamic" because the parameter list depends entirely on the user's script.

## Processing Chain Detail

1. **Event Reception** (`processHiseEvent`) - Receives HiseEvent, either processes immediately or queues for deferred execution. CPU: negligible.
2. **Note Counter Update** (`synthObject->handleNoteCounter`) - Tracks active note count for the Synth object. CPU: negligible.
3. **Callback Dispatch** (`runScriptCallbacks`) - Routes to appropriate callback based on event type. CPU: negligible (framework), depends on user script.
4. **Script Execution** (`scriptEngine->executeCallback`) - Executes the user's HiseScript callback. CPU: depends entirely on user script complexity.
5. **Timer Execution** (`runTimerCallback`) - If timer event, executes onTimer callback. CPU: depends on user script.

## Conditional Behavior

### Deferred Mode
When `deferred == true`:
- MIDI events are queued via `DeferredExecutioner` and processed asynchronously on the message thread
- Timer uses JUCE `Timer` instead of sample-accurate HiseEvent timer
- Artificial and ignored events are skipped in deferred mode
- Useful for heavy UI operations that would block the audio thread

### Front Mode
When `front == true`:
- The ScriptProcessor's Content is rendered as the plugin's main interface
- No functional difference in callback behaviour

### Empty Callback Optimization
Each callback checks `isSnippetEmpty()` before execution. If the callback body is empty, the script engine is not invoked (zero cost).

## CPU Assessment

- **Framework overhead:** negligible (event routing, callback dispatch)
- **Actual CPU cost:** entirely depends on user script complexity
- **Baseline tier:** negligible (the framework itself)
- **Scaling factors:** user script complexity, number of callbacks with code, deferred mode (adds thread synchronization overhead)

## UI Components

The ScriptProcessor uses `ScriptingEditor` as its editor component. When set as front interface, the Content is displayed via the frontend editor system.

## Notes

The ScriptProcessor is the central scripting module in HISE. It is always a MidiProcessor (placed in the MIDI processing chain of a sound generator). Despite being classified as a MIDI processor, the front interface ScriptProcessor is also responsible for the entire plugin UI, making it the most important module in most HISE projects.

The `onControl` callback has a special routing priority: if a UI component is connected to a processor parameter, macro, global cable, or has a custom control callback, the default `onControl` callback is bypassed entirely.
