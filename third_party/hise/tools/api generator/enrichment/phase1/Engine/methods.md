## addModuleStateToUserPreset

**Signature:** `void addModuleStateToUserPreset(var moduleId)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies `ModuleStateManager::modules` (heap-allocated `ReferenceCountedArray` operations, string construction, processor tree traversal).
**Minimal Example:** `Engine.addModuleStateToUserPreset("SimpleGain1");`

**Description:**
Adds an entire module's state to the user preset system. When user presets are saved, the full state (ValueTree) of the registered module is included. The module is identified by its processor ID. Passing an empty string clears all registered modules. If the module is already registered, it is re-registered (removed and re-added). Also accepts a JSON object with an `ID` property for the module identifier.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| moduleId | String | no | The processor ID to register, an empty string to clear all, or a JSON object with an `ID` property. | Must match an existing processor name, or be an empty string. |

**Pitfalls:**
- Calling this method with the same module ID again silently replaces the previous registration without any console message. Only the initial registration produces a "Added X to user preset system" console message.

**Cross References:**
- `$API.Engine.createUserPresetHandler$`
- `$API.Engine.saveUserPreset$`
- `$API.Engine.loadUserPreset$`

## allNotesOff

**Signature:** `void allNotesOff()`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets an atomic boolean flag (`allNotesOffFlag = true`). No allocations, no locks, no unbounded work.
**Minimal Example:** `Engine.allNotesOff();`

**Description:**
Sends an all-notes-off message at the next audio buffer. This does not immediately stop voices -- it sets a flag that the audio engine checks at the start of the next processing block, at which point all active voices are killed. This is a non-blocking, audio-thread-safe operation.

**Parameters:**
None.

**Cross References:**
- `$API.Synth.allNotesOff$`

## clearMidiFilePool

**Signature:** `void clearMidiFilePool()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Clears pool data structures (heap deallocation, string operations). Backend-only; compiled to a no-op in exported plugins.
**Minimal Example:** `Engine.clearMidiFilePool();`

**Description:**
Removes all cached entries from the MIDI file pool. This is a backend-only (HISE IDE) operation -- in compiled plugins the method body is empty. Prints the number of removed entries to the console. Use this to force MIDI files to be re-read from disk on next access.

**Parameters:**
None.

**Pitfalls:**
- This method is a complete no-op in exported plugins (guarded by `USE_BACKEND`). Calling it in a compiled plugin has no effect and produces no error or warning.

**Cross References:**
- `$API.Engine.clearSampleMapPool$`
- `$API.Engine.rebuildCachedPools$`

## clearSampleMapPool

**Signature:** `void clearSampleMapPool()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Clears pool data structures (heap deallocation, string operations). Backend-only; compiled to a no-op in exported plugins.
**Minimal Example:** `Engine.clearSampleMapPool();`

**Description:**
Removes all cached entries from the sample map pool. This is a backend-only (HISE IDE) operation -- in compiled plugins the method body is empty. Prints the number of removed entries to the console. Use this to force sample maps to be re-read from disk on next access.

**Parameters:**
None.

**Pitfalls:**
- This method is a complete no-op in exported plugins (guarded by `USE_BACKEND`). Calling it in a compiled plugin has no effect and produces no error or warning.

**Cross References:**
- `$API.Engine.clearMidiFilePool$`
- `$API.Engine.rebuildCachedPools$`

## clearUndoHistory

**Signature:** `void clearUndoHistory()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `UndoManager::clearUndoHistory()` which deallocates stored undo actions. Also checks `isPerformingUndoRedo()` state.
**Minimal Example:** `Engine.clearUndoHistory();`

**Description:**
Clears the undo history, removing all stored undoable actions. Throws a script error if called while an undo or redo operation is in progress.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.undo$`
- `$API.Engine.redo$`
- `$API.Engine.performUndoAction$`

## compressJSON

**Signature:** `String compressJSON(var object)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Performs JSON serialization (string allocation), zstd compression (heap allocation), and Base64 encoding (string construction).
**Minimal Example:** `var b64 = Engine.compressJSON({"key": "value"});`

**Description:**
Converts a JSON object to its string representation, compresses it using zstd compression, and encodes the result as a Base64 string. This is useful for storing complex data structures in a compact string format (e.g., for clipboard operations or custom preset data). Use `Engine.uncompressJSON()` to reverse the process.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| object | JSON | no | The JSON object to compress. | Must be a valid JSON-serializable value. |

**Cross References:**
- `$API.Engine.uncompressJSON$`
- `$API.Engine.dumpAsJSON$`

**Example:**
```javascript:compress-uncompress-roundtrip
// Title: Compress and uncompress a JSON object
var data = {"name": "MyPreset", "values": [1, 2, 3]};
var compressed = Engine.compressJSON(data);
var restored = Engine.uncompressJSON(compressed);
Console.print(restored.name);
```
```json:testMetadata:compress-uncompress-roundtrip
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof compressed", "value": "string"},
    {"type": "REPL", "expression": "restored.name", "value": "MyPreset"},
    {"type": "REPL", "expression": "restored.values[2]", "value": 3}
  ]
}
```

## copyToClipboard

**Signature:** `void copyToClipboard(String textToCopy)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `SystemClipboard::copyTextToClipboard()` which involves OS clipboard API calls (message thread interaction, potential locking).
**Minimal Example:** `Engine.copyToClipboard("Hello");`

**Description:**
Copies the given text string to the system clipboard. This wraps the JUCE `SystemClipboard::copyTextToClipboard()` function.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| textToCopy | String | no | The text to copy to the clipboard. | -- |

**Cross References:**
- `$API.Engine.getClipboardContent$`

## createAndRegisterAudioFile

**Signature:** `ScriptAudioFile createAndRegisterAudioFile(int index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptAudioFile` on the heap and registers it in the `ExternalDataHolder` slot system.
**Minimal Example:** `var af = Engine.createAndRegisterAudioFile(0);`

**Description:**
Creates a `ScriptAudioFile` object and registers it at the given slot index so it can be accessed from other modules (e.g., scriptnode nodes or other script processors) via the `ExternalData` system. The index parameter specifies which slot to register the audio file in. Multiple calls with the same index replace the previous registration. The returned object provides methods for loading audio files, accessing buffer data, setting display/content callbacks, and controlling playback ranges.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | The slot index to register the audio file in. | Zero-based index. |

**Cross References:**
- `$API.Engine.createAndRegisterSliderPackData$`
- `$API.Engine.createAndRegisterTableData$`
- `$API.Engine.createAndRegisterRingBuffer$`
- `$API.Engine.getComplexDataReference$`

## createAndRegisterSliderPackData

**Signature:** `ScriptSliderPackData createAndRegisterSliderPackData(int index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptSliderPackData` on the heap and registers it in the `ExternalDataHolder` slot system.
**Minimal Example:** `var sp = Engine.createAndRegisterSliderPackData(0);`

**Description:**
Creates a `ScriptSliderPackData` object and registers it at the given slot index so it can be accessed from other modules (e.g., scriptnode nodes or other script processors) via the `ExternalData` system. SliderPack data holds an array of float values that can be visualized and edited as a slider pack UI component. The index parameter specifies which slot to register the data in. Multiple calls with the same index replace the previous registration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | The slot index to register the slider pack data in. | Zero-based index. |

**Cross References:**
- `$API.Engine.createAndRegisterAudioFile$`
- `$API.Engine.createAndRegisterTableData$`
- `$API.Engine.createAndRegisterRingBuffer$`
- `$API.Engine.getComplexDataReference$`

## createAndRegisterTableData

**Signature:** `ScriptTableData createAndRegisterTableData(int index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptTableData` on the heap and registers it in the `ExternalDataHolder` slot system.
**Minimal Example:** `var td = Engine.createAndRegisterTableData(0);`

**Description:**
Creates a `ScriptTableData` object and registers it at the given slot index so it can be accessed from other modules (e.g., scriptnode nodes or other script processors) via the `ExternalData` system. Table data holds a lookup table curve that can be visualized and edited as a table UI component. The index parameter specifies which slot to register the data in. Multiple calls with the same index replace the previous registration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | The slot index to register the table data in. | Zero-based index. |

**Cross References:**
- `$API.Engine.createAndRegisterAudioFile$`
- `$API.Engine.createAndRegisterSliderPackData$`
- `$API.Engine.createAndRegisterRingBuffer$`
- `$API.Engine.getComplexDataReference$`

## createBroadcaster

**Signature:** `ScriptBroadcaster createBroadcaster(var defaultValues)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptBroadcaster` on the heap, registers as a callable object with the `JavascriptProcessor`, and parses the metadata/argument definitions.
**Minimal Example:** `var bc = Engine.createBroadcaster({"id": "MyBroadcaster", "args": ["value", "source"]});`

**Description:**
Creates a broadcaster that can send messages to attached listeners. The `defaultValues` parameter defines the broadcaster's argument schema and identity. The preferred format is a JSON object with `id` and `args` properties. The `args` property can be either an array of argument name strings (arguments initialized to `undefined`) or a JSON object whose keys are argument names and values are the defaults. The broadcaster also accepts a plain array of default values (argument names are auto-generated) or a single value for a one-argument broadcaster. The created broadcaster's argument count determines which `attach*` source methods are compatible.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| defaultValues | JSON | no | The broadcaster configuration: a JSON object with `id` and `args` properties, a plain array of default values, or a single default value. | When using JSON format, must have both `id` (String) and `args` (Array or Object) properties. |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| id | String | A unique identifier for the broadcaster (used for debugging and profiling). |
| args | Array or Object | Argument definitions: an array of argument name strings, or a JSON object mapping argument names to default values. |

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.sendAsyncMessage$`
- `$API.Broadcaster.attachToComponentValue$`
- `$API.Broadcaster.attachToComponentProperties$`

**Example:**
```javascript:create-broadcaster-with-listeners
// Title: Create a broadcaster and add a sync listener
const var bc = Engine.createBroadcaster({
    "id": "ValueBroadcaster",
    "args": ["value", "source"]
});

var lastValue = 0;

bc.addListener("logger", "Tracks value changes",
function(value, source)
{
    lastValue = value;
});

bc.sendSyncMessage([42, "knob"]);
```
```json:testMetadata:create-broadcaster-with-listeners
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastValue", "value": 42},
    {"type": "REPL", "expression": "bc.value", "value": 42},
    {"type": "REPL", "expression": "bc.source", "value": "knob"}
  ]
}
```

## createDspNetwork

**Signature:** `DspNetwork createDspNetwork(String id)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `DspNetwork` on the heap (via `Holder::getOrCreate`), parses XML files in backend, sets up ValueTree structures, and modifies the holder's network list.
**Minimal Example:** `var network = Engine.createDspNetwork("MyNetwork");`

**Description:**
Creates or retrieves a scriptnode DSP network with the given ID. The script processor must be a `DspNetwork::Holder` (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator) -- if it is not, the method reports a script error. If a network with the same ID already exists on this processor, it is returned and set as the active network (no duplicate is created). In the backend, the method checks for existing `.xml` network files in the DspNetworks folder and loads from file if found. A new network starts with an empty `container.chain` root node.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | The unique identifier for the DSP network. | Must be unique per processor. In backend, matches against `.xml` filenames in the DspNetworks folder. |

**Pitfalls:**
- Calling this on a script processor that is not a `DspNetwork::Holder` (e.g., a plain Script Processor or Script Voice Start Modulator) produces a script error "Not available on this script processor". Only ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, and ScriptEnvelopeModulator can create DSP networks.

**Cross References:**
- `$API.Engine.getDspNetworkReference$`

## createBeatportManager

**Signature:** `BeatportManager createBeatportManager()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `BeatportManager` on the heap.
**Minimal Example:** `var bp = Engine.createBeatportManager();`

**Description:**
Creates a Beatport manager object for integration with the Beatport streaming platform. The returned object provides methods for authenticating, browsing, and managing content through the Beatport API.

**Parameters:**
None.

**Cross References:**
None.

## getBufferSize

**Signature:** `int getBufferSize()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Returns a cached `int` member variable (`largestBlockSize`) from the Processor. No allocations, no locks.
**Minimal Example:** `var bs = Engine.getBufferSize();`

**Description:**
Returns the current maximum processing block size in samples. This is the largest audio buffer size that the processor has been prepared with -- typically set by the host/audio driver. This value is useful for allocating buffers or calculating time-based parameters. Note that this returns the processor's `largestBlockSize`, which may differ from the host's actual buffer size if `Engine.setMaximumBlockSize()` has been called to split incoming buffers into smaller chunks.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getSampleRate$`
- `$API.Engine.setMaximumBlockSize$`

## getClipboardContent

**Signature:** `String getClipboardContent()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `SystemClipboard::getTextFromClipboard()` which involves OS clipboard API calls (message thread interaction, string allocation).
**Minimal Example:** `var text = Engine.getClipboardContent();`

**Description:**
Returns the current text content of the system clipboard as a string. This wraps the JUCE `SystemClipboard::getTextFromClipboard()` function. Returns an empty string if the clipboard is empty or does not contain text. Useful for implementing paste functionality in custom UI elements or for importing data from external applications.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.copyToClipboard$`

## createBackgroundTask

**Signature:** `ScriptBackgroundTask createBackgroundTask(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptBackgroundTask` on the heap (which extends `Thread`), sets up `WeakCallbackHolder` objects, and registers a pre-compile listener.
**Minimal Example:** `var task = Engine.createBackgroundTask("MyTask");`

**Description:**
Creates a background task object that can execute heavyweight functions on a separate thread without blocking the audio or UI thread. The `name` parameter is used as the thread name for debugging purposes. The returned `ScriptBackgroundTask` object provides methods to run processes (`runProcess`), set finish callbacks (`setFinishCallback`), track progress (`setProgress`/`getProgress`), send abort signals (`sendAbortSignal`/`shouldAbort`), and forward status to the loading thread. The background thread is automatically aborted when the script is recompiled.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | The name of the background thread (used for debugging/profiling). | -- |

**Cross References:**
- `$API.Engine.renderAudio$`

## createBXLicenser

**Signature:** `ScriptBXLicenser createBXLicenser()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptBXLicenser` on the heap. Guarded by `HISE_INCLUDE_BX_LICENSER` preprocessor flag.
**Minimal Example:** `var bx = Engine.createBXLicenser();`

**Description:**
Creates a BX Licenser object for copy protection using the proprietary BX SDK. Requires the `HISE_INCLUDE_BX_LICENSER` preprocessor flag to be enabled at compile time. If the flag is not set, the method reports a script error ("BX Licenser is not enabled") and returns undefined.

**Parameters:**
None.

**Pitfalls:**
- Calling this method without the `HISE_INCLUDE_BX_LICENSER` preprocessor flag enabled throws a script error at runtime. This flag must be set in the project's extra definitions before export.

**Cross References:**
- `$API.Engine.createLicenseUnlocker$`
- `$API.Engine.createNKSManager$`

## createAndRegisterRingBuffer

**Signature:** `ScriptRingBuffer createAndRegisterRingBuffer(int index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptRingBuffer` on the heap and registers it in the `ExternalDataHolder` slot system.
**Minimal Example:** `var rb = Engine.createAndRegisterRingBuffer(0);`

**Description:**
Creates a `ScriptRingBuffer` object and registers it at the given slot index so it can be accessed from other modules (e.g., scriptnode nodes or other script processors) via the `ExternalData` system. Ring buffers are display buffers that hold a circular buffer of audio data, useful for oscilloscope-style visualizations or FFT displays. The index parameter specifies which slot to register the ring buffer in. Multiple calls with the same index replace the previous registration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | The slot index to register the ring buffer in. | Zero-based index. |

**Cross References:**
- `$API.Engine.createAndRegisterAudioFile$`
- `$API.Engine.createAndRegisterSliderPackData$`
- `$API.Engine.createAndRegisterTableData$`
- `$API.Engine.getComplexDataReference$`

## createErrorHandler

**Signature:** `ScriptErrorHandler createErrorHandler()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptErrorHandler` on the heap.
**Minimal Example:** `var eh = Engine.createErrorHandler();`

**Description:**
Creates a `ScriptErrorHandler` object that can react to initialization errors and compilation failures. The error handler provides a scripting interface for intercepting and customizing error reporting behavior.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.showErrorMessage$`

## createExpansionHandler

**Signature:** `ScriptExpansionHandler createExpansionHandler()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptExpansionHandler` on the heap. The constructor takes a `JavascriptProcessor*`, performing a `dynamic_cast` from the script processor.
**Minimal Example:** `var exh = Engine.createExpansionHandler();`

**Description:**
Creates and activates the expansion handler, which provides scripting access to the expansion pack system. The expansion handler allows installing, loading, and managing expansion packs, listening for expansion changes, and querying expansion properties. This is the primary entry point for the expansion system in HISEScript. The returned object wraps the engine's internal `ExpansionHandler` and registers the calling script processor as an expansion listener.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getExpansionList$`
- `$API.Engine.setCurrentExpansion$`

## createFFT

**Signature:** `ScriptFFT createFFT()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptFFT` on the heap.
**Minimal Example:** `var fft = Engine.createFFT();`

**Description:**
Creates an FFT (Fast Fourier Transform) object for performing frequency-domain analysis on audio buffers. The `ScriptFFT` object provides methods for forward and inverse FFT operations, enabling spectral analysis and manipulation in HISEScript.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.createAndRegisterRingBuffer$`

## createFixObjectFactory

**Signature:** `fixobj::Factory createFixObjectFactory(var layoutDescription)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `fixobj::Factory` on the heap, creates an `Allocator`, and parses the layout description into `MemoryLayoutItem` objects.
**Minimal Example:** `var factory = Engine.createFixObjectFactory({"x": 0, "y": 0, "active": false});`

**Description:**
Creates a fixed-layout object factory using the given data layout description. The factory produces memory-efficient, fixed-layout objects with typed fields (Integer, Boolean, Float) that share a common binary layout. These objects are more performant than regular JSON objects for large collections because they use contiguous memory with known offsets rather than hash-map lookups. The layout description is a JSON object where property names become field names and property values define the type and default value -- `0` or any integer for Integer fields, `0.0` or any float for Float fields, `false`/`true` for Boolean fields, or an array of those for array-typed fields. The factory stores the layout as a `prototype` constant, accessible via `factory.prototype`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layoutDescription | JSON | no | A JSON object defining field names, types, and default values. | Must be a DynamicObject (not an array or primitive). An empty or non-object value causes an internal "No data" error. |

**Pitfalls:**
- [BUG] If `layoutDescription` is not a JSON object (e.g., an array, string, or number), the factory is created but all subsequent `create()`, `createArray()`, and `createStack()` calls silently return undefined. No error message is reported to the user.

**Example:**
```javascript:fixobj-factory-basic
// Title: Creating and using a fixed-layout object factory
const var pointFactory = Engine.createFixObjectFactory({
    "x": 0.0,
    "y": 0.0,
    "active": false
});

var point = pointFactory.create();
point.x = 1.5;
point.y = 2.5;
point.active = true;

Console.print(point.x); // 1.5
```
```json:testMetadata:fixobj-factory-basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "point.x", "value": 1.5},
    {"type": "REPL", "expression": "point.active", "value": true}
  ]
}
```

**Cross References:**
- `$API.FixObjectFactory.create$`
- `$API.FixObjectFactory.createArray$`
- `$API.FixObjectFactory.createStack$`

## createGlobalScriptLookAndFeel

**Signature:** `ScriptedLookAndFeel createGlobalScriptLookAndFeel()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** May allocate a new `ScriptedLookAndFeel` on the heap, or return a reference to the existing singleton. The singleton check accesses `MainController::getCurrentScriptLookAndFeel()`.
**Minimal Example:** `var laf = Engine.createGlobalScriptLookAndFeel();`

**Description:**
Creates a global script look-and-feel object, or returns the existing one if it has already been created. The global LAF applies styling to all components in the plugin that do not have a local look-and-feel override. This is a singleton-or-create pattern -- the first call creates the object with `isGlobal=true` and registers it with the `MainController`; subsequent calls from any script processor return the same instance. Use `laf.registerFunction()` to register custom drawing callbacks for various UI elements (buttons, sliders, combo boxes, preset browser, etc.).

**Parameters:**
None.

**Pitfalls:**
- Multiple script processors can access the same global LAF instance. Registering a drawing function in one processor overwrites any previous registration for that function name made by another processor. The last registration wins with no warning.

**Cross References:**
- `$API.Content.createLocalLookAndFeel$`

## createLicenseUnlocker

**Signature:** `ScriptUnlocker createLicenseUnlocker()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptUnlocker::RefObject` on the heap.
**Minimal Example:** `var unlocker = Engine.createLicenseUnlocker();`

**Description:**
Creates a reference to the script license manager (copy protection system). The returned object provides methods for checking license status, registering product keys, and managing trial/demo modes. This is the scripting interface for HISE's built-in RSA-based copy protection. Requires the `USE_COPY_PROTECTION` preprocessor flag to be enabled in the project settings for the license system to be functional.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getProjectInfo$`

## createMacroHandler

**Signature:** `ScriptedMacroHandler createMacroHandler()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptedMacroHandler` on the heap.
**Minimal Example:** `var mh = Engine.createMacroHandler();`

**Description:**
Creates a macro handler that provides scripting access to HISE's macro control system. The macro handler allows programmatically changing macro connections, setting macro values, and querying which parameters are connected to each macro slot. HISE supports up to `HISE_NUM_MACROS` macros (default 8). Macros must first be enabled via `Engine.setFrontendMacros()` before the macro handler can manage connections.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.setFrontendMacros$`
- `$API.Engine.getMacroName$`

## createMessageHolder

**Signature:** `ScriptingMessageHolder createMessageHolder()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptingMessageHolder` on the heap.
**Minimal Example:** `var msg = Engine.createMessageHolder();`

**Description:**
Creates a storage object for MIDI message events. A `ScriptingMessageHolder` wraps a `HiseEvent` and provides methods for reading and writing MIDI event properties (note number, velocity, channel, controller values, etc.). Message holders are used to construct MIDI events for `Engine.renderAudio()`, to store and manipulate MIDI data outside of real-time callbacks, and to pass structured MIDI data between different parts of a script. Unlike the implicit `Message` object in MIDI callbacks, a message holder persists across callbacks and can be stored in arrays or variables.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.renderAudio$`
- `$API.Message$`

## createMidiAutomationHandler

**Signature:** `var createMidiAutomationHandler()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptedMidiAutomationHandler` on the heap.
**Minimal Example:** `var mah = Engine.createMidiAutomationHandler();`

**Description:**
Creates a MIDI automation handler object that provides script-level access to the `MidiControlAutomationHandler` system. This handler manages the mapping between MIDI CC messages and plugin parameters, allowing scripts to programmatically configure, query, and modify MIDI learn assignments. The returned `ScriptedMidiAutomationHandler` wraps the `MainController`'s automation handler and exposes methods for connecting MIDI controllers to parameters, managing automation slots, and handling MPE data.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.isControllerUsedByAutomation$`
- `$API.Engine.createUserPresetHandler$`

## createMidiList

**Signature:** `MidiList createMidiList()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `MidiList` on the heap.
**Minimal Example:** `var ml = Engine.createMidiList();`

**Description:**
Creates a MIDI list object -- a fixed-size array of 128 integer values (one per MIDI note number, indices 0-127). `MidiList` is a lightweight container optimized for MIDI-related lookups such as key-to-velocity mappings, note filtering masks, or custom note-to-value tables. Unlike a regular HISEScript array, `MidiList` supports fast serialization via `restoreFromBase64String`/`getBase64String` and can be stored in user presets. Values default to -1, which conventionally means "unused" or "disabled".

**Parameters:**
None.

**Cross References:**
- `$API.MidiList$`

## createModulationMatrix

**Signature:** `var createModulationMatrix(String containerId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptModulationMatrix` on the heap; constructor performs processor lookup via string ID.
**Minimal Example:** `var mm = Engine.createModulationMatrix("GlobalModulatorContainer1");`

**Description:**
Creates a modulation matrix object that manages dynamic modulation routing using a Global Modulator Container as the modulation source. The `containerId` parameter must be the processor ID of an existing `GlobalModulatorContainer` module in the signal chain. The returned `ScriptModulationMatrix` provides methods for connecting modulators to target parameters, managing modulation slots, and controlling modulation intensity. This is used for building user-facing modulation assignment interfaces (drag-and-drop modulation routing).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| containerId | String | no | The processor ID of a `GlobalModulatorContainer` module | Must match an existing module ID |

**Pitfalls:**
- If the `containerId` does not match any `GlobalModulatorContainer` in the module tree, the constructor of `ScriptModulationMatrix` will fail to find the container. The behavior depends on the internal validation of the `ScriptModulationMatrix` constructor.

**Cross References:**
- `$API.ScriptModulationMatrix$`

## createNKSManager

**Signature:** `var createNKSManager()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptNKSManager` on the heap (when the SDK is available).
**Minimal Example:** `var nks = Engine.createNKSManager();`

**Description:**
Creates an NKS (Native Kontrol Standard) manager object for integration with Native Instruments hardware controllers and the NKS ecosystem. This method is only available when the project is compiled with the `HISE_INCLUDE_NKS_SDK` preprocessor flag enabled. If the NKS SDK is not included in the build, calling this method throws a script error: "NKS support is not enabled".

**Parameters:**
None.

**Pitfalls:**
- Throws a script error at runtime if `HISE_INCLUDE_NKS_SDK` is not defined in the build configuration. There is no way to check at script level whether NKS support is compiled in before calling this method.

**Cross References:**
- `$API.Engine.createBXLicenser$`

**Related Preprocessors:**
`HISE_INCLUDE_NKS_SDK`

## createNeuralNetwork

**Signature:** `ScriptNeuralNetwork createNeuralNetwork(String id)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptNeuralNetwork` on the heap.
**Minimal Example:** `var nn = Engine.createNeuralNetwork("MyModel");`

**Description:**
Creates a neural network object with the given string identifier. The `ScriptNeuralNetwork` wraps RTNeural, a lightweight inference engine for pre-trained neural network models. The `id` parameter serves as a unique identifier for this network instance. After creation, the network must be loaded with a trained model before it can process audio data. Neural networks are typically used for amp modeling, effects emulation, or other ML-based audio processing tasks within scriptnode.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | A unique identifier for the neural network instance | -- |

**Cross References:**
- `$API.ScriptNeuralNetwork$`

## createThreadSafeStorage

**Signature:** `var createThreadSafeStorage()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptThreadSafeStorage` on the heap.
**Minimal Example:** `var tss = Engine.createThreadSafeStorage();`

**Description:**
Creates a thread-safe storage container that enables safe data exchange between the audio thread and the scripting/UI thread. The `ScriptThreadSafeStorage` provides lock-free read/write methods that allow the audio thread to store data (e.g., parameter values, analysis results) that can then be safely read from the UI thread for display or further processing, and vice versa. This avoids the need for locks or unsafe shared variable access across threads.

**Parameters:**
None.

**Cross References:**
- `$API.ScriptThreadSafeStorage$`

## createTimerObject

**Signature:** `TimerObject createTimerObject()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `TimerObject` on the heap.
**Minimal Example:** `var timer = Engine.createTimerObject();`

**Description:**
Creates a timer object that provides periodic callback execution at a configurable interval. The `TimerObject` is the HISEScript equivalent of `setInterval()`/`setTimeout()` from JavaScript. After creation, use `setTimerCallback(callback)` to register a function and `startTimer(intervalMs)` to begin periodic execution. The callback runs on the UI thread (not the audio thread), making it suitable for display updates, polling, animation, and deferred processing. Multiple independent timer objects can be created for different periodic tasks.

**Parameters:**
None.

**Cross References:**
- `$API.TimerObject$`

## createTransportHandler

**Signature:** `var createTransportHandler()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `TransportHandler` on the heap.
**Minimal Example:** `var th = Engine.createTransportHandler();`

**Description:**
Creates a transport handler object that provides callback-based access to host transport events (play/stop, tempo changes, time signature, beat position). The `TransportHandler` is the modern replacement for `Engine.getPlayHead()`, which returns a stale/empty object due to commented-out property-population code. The transport handler registers as a `TempoListener` and receives real-time updates from the host DAW or from the internal clock in standalone mode. Use it to synchronize script behavior to the host transport -- e.g., triggering actions on beat boundaries, reacting to tempo changes, or implementing tempo-synced UI animations.

**Parameters:**
None.

**Cross References:**
- `$API.TransportHandler$`
- `$API.Engine.getPlayHead$`
- `$API.Engine.getHostBpm$`

## createUnorderedStack

**Signature:** `ScriptUnorderedStack createUnorderedStack()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptUnorderedStack` on the heap.
**Minimal Example:** `var stack = Engine.createUnorderedStack();`

**Description:**
Creates an unordered stack data structure that can hold up to 128 float numbers. The `ScriptUnorderedStack` is a fixed-capacity, unordered collection optimized for audio-thread-safe operations. It supports insert, remove, contains, and clear operations without heap allocation after creation. Unlike a regular array, the stack does not preserve insertion order -- elements may be rearranged when items are removed. This makes it suitable for tracking active note numbers, voice IDs, or other numeric sets where order does not matter and lock-free performance is required.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.createMidiList$`

## createUserPresetHandler

**Signature:** `ScriptUserPresetHandler createUserPresetHandler()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a `ScriptUserPresetHandler` on the heap.
**Minimal Example:** `var uph = Engine.createUserPresetHandler();`

**Description:**
Creates a user preset handler object that provides scripting access to the user preset lifecycle. The `ScriptUserPresetHandler` exposes methods for intercepting preset load/save events, implementing custom preset models (where the script manages serialization instead of the default system), attaching pre/post callbacks to preset changes, and querying or modifying the preset state programmatically. This is the central object for customizing how user presets behave in a plugin -- e.g., adding confirmation dialogs before loading, performing custom data migration on load, or filtering which parameters are saved.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.loadUserPreset$`
- `$API.Engine.saveUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`
- `$API.Engine.addModuleStateToUserPreset$`

## decodeBase64ValueTree

**Signature:** `String decodeBase64ValueTree(String b64Data)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Performs Base64 decoding, zstd decompression attempts, ValueTree parsing, and XML document construction -- all involving heap allocations and string operations.
**Minimal Example:** `var xml = Engine.decodeBase64ValueTree(b64String);`

**Description:**
Decodes a Base64-encoded ValueTree (such as a HISE snippet) and returns its content as an XML string. The method attempts three decoding strategies in sequence: (1) `ValueTreeConverters::convertBase64ToValueTree` (the standard HISE format), (2) zstd decompression followed by ValueTree parsing, and (3) raw Base64 decoding into a MemoryBlock followed by `ValueTree::readFromData`. The first strategy that produces a valid ValueTree wins. If all three strategies fail, returns an empty string. This is primarily a debugging/inspection utility -- useful for examining the contents of HISE snippets, encoded preset data, or any Base64-encoded ValueTree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64Data | String | no | A Base64-encoded string containing a ValueTree. | Must be valid Base64. |

**Pitfalls:**
- Returns an empty string (not an error) when the input cannot be decoded by any of the three strategies. There is no way to distinguish between "successfully decoded an empty tree" and "all decoding attempts failed" from the return value alone.

**Cross References:**
- `$API.Engine.compressJSON$`
- `$API.Engine.uncompressJSON$`

## doubleToString

**Signature:** `String doubleToString(double value, int digits)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a new `String` object from a double (heap allocation for the string buffer).
**Minimal Example:** `var s = Engine.doubleToString(3.14159, 2);`

**Description:**
Returns a string representation of the given double value with the specified number of decimal digits. Delegates directly to `String(value, digits)` (the JUCE String constructor that formats a double). The `digits` parameter controls the number of decimal places shown. This is a formatting utility for display purposes -- e.g., showing parameter values in labels or building formatted text for UI elements.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Double | no | The numeric value to convert. | -- |
| digits | Integer | no | The number of decimal places to include. | 0 or positive integer. |

**Cross References:**
- `$API.Engine.intToHexString$`
- `$API.Engine.getTextForValue$`

## dumpAsJSON

**Signature:** `void dumpAsJSON(var object, String fileName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Performs JSON serialization (string allocation), file path resolution, and disk I/O (`File::replaceWithText`).
**Minimal Example:** `Engine.dumpAsJSON({"key": "value"}, "myData.json");`

**Description:**
Exports a JSON object to a file. If `fileName` is an absolute path, writes to that location directly. If `fileName` is a relative path, it is resolved relative to the project's UserPresets directory. The object is serialized using `JSON::toString()` with up to 8 decimal digits for floating-point values. Only objects (JSON/DynamicObject) can be exported -- passing a non-object value (array, string, number) causes a script error: "Only objects can be exported as JSON". Use `Engine.loadFromJSON()` to read the file back.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| object | JSON | no | The JSON object to export. | Must be a DynamicObject (not an array or primitive). |
| fileName | String | no | The output file path. Relative paths resolve to the UserPresets directory. | -- |

**Pitfalls:**
- Only DynamicObjects (JSON objects) are accepted. Passing an Array causes a script error, even though arrays are valid JSON. To export an array, wrap it in an object: `Engine.dumpAsJSON({"data": myArray}, "file.json")`.

**Cross References:**
- `$API.Engine.loadFromJSON$`
- `$API.Engine.compressJSON$`

**Example:**
```javascript:dump-and-load-json
// Title: Save and load a JSON configuration file
var config = {"volume": 0.8, "mode": "stereo", "channels": 2};
Engine.dumpAsJSON(config, "myConfig.json");
var loaded = Engine.loadFromJSON("myConfig.json");
Console.print(loaded.volume);
```
```json:testMetadata:dump-and-load-json
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "loaded.volume", "value": 0.8},
    {"type": "REPL", "expression": "loaded.mode", "value": "stereo"},
    {"type": "REPL", "expression": "loaded.channels", "value": 2}
  ]
}
```

## extendTimeOut

**Signature:** `void extendTimeOut(int additionalMilliseconds)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Adds an integer to a `Time` member variable (simple arithmetic on an int64 value type). No allocations, no locks.
**Minimal Example:** `Engine.extendTimeOut(10000);`

**Description:**
Extends the compilation timeout by the given number of milliseconds. Use this during `onInit` if a long-running initialization task (e.g., loading large datasets, building lookup tables) would otherwise exceed HISE's default compilation timeout and trigger a timeout error. In compiled (exported) plugins, this method has no practical effect because there is no active compilation timeout -- the script initialization runs without a time limit in the frontend.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| additionalMilliseconds | Integer | no | The number of milliseconds to add to the current compilation timeout. | Positive integer. |

**Cross References:**
None.

## getComplexDataReference

**Signature:** `var getComplexDataReference(String dataType, String moduleId, int index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new scripting wrapper object on the heap (`new ScriptTableData`, etc.) and performs processor tree lookup via `ProcessorHelpers::getFirstProcessorWithName`.
**Minimal Example:** `var table = Engine.getComplexDataReference("Table", "LFO Modulator1", 0);`

**Description:**
Returns a scripting reference to a complex data object (Table, SliderPack, AudioFile, or DisplayBuffer) owned by another module in the processor tree. This allows cross-module access to complex data -- for example, reading or modifying a table curve that belongs to a different processor. The module is looked up by its processor ID, and the data slot is selected by a zero-based index. If the requested data slot does not exist on the module, the method silently returns `undefined` without reporting an error. If the module itself is not found, a script error is reported.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataType | String | no | The type of complex data to retrieve. | Must be one of: `"Table"`, `"SliderPack"`, `"AudioFile"`, `"DisplayBuffer"` |
| moduleId | String | no | The processor ID of the module that owns the data. | Must match an existing `ExternalDataHolder` module in the processor tree. |
| index | Integer | no | Zero-based index of the data slot on the module. | >= 0. Returns `undefined` if out of range. |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"Table"` | Returns a `ScriptTableData` reference to the module's table at the given index. |
| `"SliderPack"` | Returns a `ScriptSliderPackData` reference to the module's slider pack at the given index. |
| `"AudioFile"` | Returns a `ScriptAudioFile` reference to the module's audio file slot at the given index. |
| `"DisplayBuffer"` | Returns a `ScriptRingBuffer` reference to the module's display buffer at the given index. |

**Pitfalls:**
- If the data slot index exceeds the number of slots the module provides, the method silently returns `undefined` with no error message. Check the return value with `isDefined()` before using it.
- [BUG] The `dataType` parameter also accepts `"FilterCoefficients"` (it passes the internal validation check), but there is no corresponding return-value case in the switch statement. Passing `"FilterCoefficients"` silently returns `undefined` in release builds and hits a debug assertion in debug builds.
- The error message for invalid `dataType` values ("Must be Table, SliderPack, AudioFile or DisplayBuffer") does not mention `"FilterCoefficients"`, even though it is accepted by the validation.

**Cross References:**
- `$API.Engine.createAndRegisterTableData$`
- `$API.Engine.createAndRegisterSliderPackData$`
- `$API.Engine.createAndRegisterAudioFile$`
- `$API.Engine.createAndRegisterRingBuffer$`

**Example:**
```javascript:cross-module-table-access
// Title: Access a table curve from another module
var tableRef = Engine.getComplexDataReference("Table", "LFO Modulator1", 0);

if (isDefined(tableRef))
    Console.print("Table point count: " + tableRef.getTablePointsAsJSON().length);
else
    Console.print("Table not found at that index");
```
```json:testMetadata:cross-module-table-access
{
  "testable": false,
  "skipReason": "Requires an existing LFO Modulator module in the processor tree with a table data slot."
}
```

## getControlRateDownsamplingFactor

**Signature:** `double getControlRateDownsamplingFactor()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Returns a compile-time constant (`HISE_EVENT_RASTER`). No allocations, no locks, no runtime computation.
**Minimal Example:** `var factor = Engine.getControlRateDownsamplingFactor();`

**Description:**
Returns the downsampling factor used for the modulation signal (control rate). This is a compile-time constant defined by `HISE_EVENT_RASTER`, which defaults to 8. It means the modulation system processes at `sampleRate / factor` Hz. For example, at 44100 Hz sample rate with factor 8, the control rate is 5512.5 Hz. This value is useful for calculating the effective modulation update rate or for converting between sample-accurate and control-rate timing.

**Parameters:**

None.

**Cross References:**
- `$API.Engine.getSampleRate$`

## getCpuUsage

**Signature:** `double getCpuUsage()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a cached numeric value from `MainController::getCpuUsage()`. No allocations, no locks.
**Minimal Example:** `var cpu = Engine.getCpuUsage();`

**Description:**
Returns the current CPU usage of the audio engine as a percentage value in the range 0.0 to 100.0. This value is updated internally by the audio processing system and represents the fraction of available audio processing time being consumed. Useful for displaying a CPU meter in the plugin interface or for adaptive quality adjustments.

**Parameters:**

None.

**Cross References:**
- `$API.Engine.getMemoryUsage$`
- `$API.Engine.getNumVoices$`

## getCurrentUserPresetName

**Signature:** `String getCurrentUserPresetName()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a `String` constructed from the file system path.
**Minimal Example:** `var name = Engine.getCurrentUserPresetName();`

**Description:**
Returns the filename (without the `.preset` extension) of the currently loaded user preset. If no user preset has been loaded, returns an empty string. This is the bare filename only, not the full relative path -- for example, if the preset is at `Category/MyPreset.preset`, this returns `"MyPreset"`, not `"Category/MyPreset"`.

**Parameters:**

None.

**Pitfalls:**
- Returns only the filename without directory path. If presets in different subdirectories share the same name, this method cannot distinguish between them. Use `Engine.getUserPresetList()` to get full relative paths if disambiguation is needed.

**Cross References:**
- `$API.Engine.loadUserPreset$`
- `$API.Engine.getUserPresetList$`
- `$API.Engine.loadNextUserPreset$`
- `$API.Engine.loadPreviousUserPreset$`

## getDecibelsForGainFactor

**Signature:** `double getDecibelsForGainFactor(double gainFactor)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure math function (`Decibels::gainToDecibels`). No allocations, no locks. Implemented inline in the header.
**Minimal Example:** `var db = Engine.getDecibelsForGainFactor(0.5);`

**Description:**
Converts a linear gain factor to decibels. A gain of 1.0 returns 0.0 dB, a gain of 0.5 returns approximately -6.02 dB, and a gain of 0.0 returns -100.0 dB (the JUCE minimum threshold). Values above 1.0 produce positive dB values. Delegates to JUCE's `Decibels::gainToDecibels<double>()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| gainFactor | Number | yes | The linear gain factor to convert. | Typically 0.0 to 1.0 for attenuation, > 1.0 for boost. 0.0 returns -100.0 dB. |

**Cross References:**
- `$API.Engine.getGainFactorForDecibels$`

## getDeviceResolution

**Signature:** `var getDeviceResolution()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new `Array<var>` on the heap. Calls `HiseDeviceSimulator::getDisplayResolution()` which may query the OS display API.
**Minimal Example:** `var res = Engine.getDeviceResolution();`

**Description:**
Returns the full screen resolution of the current device as an array with four elements: `[x, y, width, height]`, where `x` and `y` are the top-left coordinates and `width` and `height` are the screen dimensions in pixels. On desktop systems, this returns the primary display resolution. The values come from `HiseDeviceSimulator::getDisplayResolution()`.

**Parameters:**

None.

**Cross References:**
- `$API.Engine.getDeviceType$`

## getDeviceType

**Signature:** `String getDeviceType()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a `String` from a static lookup.
**Minimal Example:** `var device = Engine.getDeviceType();`

**Description:**
Returns the device type the software is running on as a string. On desktop platforms, this always returns `"Desktop"`. The device simulator in the HISE backend can override this value for testing mobile layouts. The return value is one of the `HiseDeviceSimulator::DeviceType` enum names.

**Parameters:**

None.

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"Desktop"` | Running on a desktop computer (Windows, macOS, Linux). |
| `"iPad"` | Running on an iPad (standalone). |
| `"iPadAUv3"` | Running on an iPad as an AUv3 plugin. |
| `"iPhone"` | Running on an iPhone (standalone). |
| `"iPhoneAUv3"` | Running on an iPhone as an AUv3 plugin. |

**Cross References:**
- `$API.Engine.getDeviceResolution$`
- `$API.Engine.getOS$`

## getDspNetworkReference

**Signature:** `var getDspNetworkReference(String processorId, String id)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Iterates the processor tree, performs string comparisons, and may allocate a `DspNetwork` via `getOrCreate()`.
**Minimal Example:** `var network = Engine.getDspNetworkReference("ScriptFX1", "MyNetwork");`

**Description:**
Returns a reference to a DSP network owned by another script processor. This allows cross-processor access to scriptnode networks -- for example, querying or modifying node parameters in a network that lives in a different script module. The method iterates all `DspNetwork::Holder` processors in the module tree and matches by processor ID, then looks up the network by its name ID. If the processor is found but the network ID does not exist on it, a script error is reported. If no processor with the given ID is found among `DspNetwork::Holder` processors, the method silently returns `undefined`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| processorId | String | no | The processor ID of the script module that owns the DSP network. | Must match a `DspNetwork::Holder` processor in the module tree. |
| id | String | no | The name ID of the DSP network to retrieve. | Must be a valid network ID registered on the target processor. |

**Pitfalls:**
- If the `processorId` does not match any `DspNetwork::Holder` processor, the method silently returns `undefined` with no error message. Only a wrong `id` on a found processor produces a script error. Check the return value with `isDefined()` if the processor might not exist.

**Cross References:**
- `$API.Engine.createDspNetwork$`

## getExpansionList

**Signature:** `var getExpansionList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates a temporary `ScriptExpansionHandler` object (heap allocation), iterates the expansion list, and creates `ScriptExpansionReference` objects (heap allocations) for each expansion.
**Minimal Example:** `var expansions = Engine.getExpansionList();`

**Description:**
Returns an array of `Expansion` references representing all currently available expansions. Internally, this creates a temporary `ScriptExpansionHandler`, calls its `getExpansionList()` method, and returns the result. Each element in the returned array is a scripting reference to an `Expansion` object that can be used to query expansion properties (name, folder, properties) and manage expansion-specific resources.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.createExpansionHandler$`
- `$API.Engine.setCurrentExpansion$`
- `$API.ExpansionHandler.getExpansionList$`

## getExtraDefinitionsInBackend

**Signature:** `var getExtraDefinitionsInBackend()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Reads project settings, tokenizes strings, constructs a `DynamicObject` with property allocations. Backend-only; returns an empty object in compiled plugins.
**Minimal Example:** `var defs = Engine.getExtraDefinitionsInBackend();`

**Description:**
Returns the platform-specific extra preprocessor definitions from the Project settings as a JSON object. Each key-value pair in the returned object corresponds to one definition entry (e.g., `MY_FLAG=1` becomes `{"MY_FLAG": "1"}`). The definitions are read from the platform-appropriate project setting (`ExtraDefinitionsWindows`, `ExtraDefinitionsOSX`, or `ExtraDefinitionsLinux`). Definitions can be delimited by commas, semicolons, or newlines. In compiled plugins (frontend builds), this method returns an empty object.

**Parameters:**
None.

**Pitfalls:**
- In compiled plugins, this method silently returns an empty object (`{}`) rather than returning the definitions that were active at compile time. The "InBackend" suffix in the method name hints at this, but users may still expect the compile-time definitions to be available. Use `Engine.isHISE()` to guard calls if needed.

**Cross References:**
- `$API.Engine.isHISE$`

## getFilterModeList

**Signature:** `var getFilterModeList()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new `FilterModeObject` on the heap (a `ConstScriptingObject` with 18 integer constants registered via `addConstant()`).
**Minimal Example:** `var filters = Engine.getFilterModeList();`

**Description:**
Returns a `FilterModes` object containing named constants for all available filter modes. Each constant maps a filter type name to its integer index, which can be passed to effect modules that accept a filter mode parameter. The returned object contains these constants:

| Constant | Description |
|----------|-------------|
| `LowPass` | Standard low-pass filter |
| `HighPass` | Standard high-pass filter |
| `LowShelf` | Low shelf EQ |
| `HighShelf` | High shelf EQ |
| `Peak` | Parametric peak/bell EQ |
| `ResoLow` | Resonant low-pass |
| `StateVariableLP` | State variable low-pass |
| `StateVariableHP` | State variable high-pass |
| `MoogLP` | Moog-style ladder low-pass |
| `OnePoleLowPass` | One-pole low-pass (6dB/oct) |
| `OnePoleHighPass` | One-pole high-pass (6dB/oct) |
| `StateVariablePeak` | State variable peak |
| `StateVariableNotch` | State variable notch |
| `StateVariableBandPass` | State variable band-pass |
| `Allpass` | All-pass filter |
| `LadderFourPoleLP` | 4-pole ladder low-pass |
| `LadderFourPoleHP` | 4-pole ladder high-pass |
| `RingMod` | Ring modulation |

**Parameters:**
None.

**Cross References:**
- `$API.Effect.setAttribute$`

**Example:**
```javascript:filter-mode-lookup
// Title: Using filter mode constants from the filter mode list
const var filterModes = Engine.getFilterModeList();
const var myFilter = Synth.getEffect("PolyphonicFilter1");
myFilter.setAttribute(myFilter.Mode, filterModes.StateVariableLP);
```
```json:testMetadata:filter-mode-lookup
{
  "testable": false,
  "skipReason": "Requires a PolyphonicFilter module in the module tree."
}
```

## getFrequencyForMidiNoteNumber

**Signature:** `double getFrequencyForMidiNoteNumber(int midiNumber)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure inline math: delegates to `MidiMessage::getMidiNoteInHertz()` which computes `440 * pow(2, (note - 69) / 12.0)`. No allocations, no locks.
**Minimal Example:** `var freq = Engine.getFrequencyForMidiNoteNumber(69);`

**Description:**
Converts a MIDI note number to its corresponding frequency in Hertz using the standard A440 tuning reference. MIDI note 69 (A4) returns 440.0 Hz. The conversion uses the standard equal temperament formula. Values outside the 0-127 range are not clamped -- the formula still computes a valid frequency for any integer input.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| midiNumber | Number | yes | The MIDI note number to convert. | Typically 0-127, but not range-checked. |

**Cross References:**
- `$API.Engine.getMidiNoteName$`
- `$API.Engine.getMidiNoteFromName$`
- `$API.Engine.getPitchRatioFromSemitones$`

## getGainFactorForDecibels

**Signature:** `double getGainFactorForDecibels(double decibels)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure inline math: delegates to `Decibels::decibelsToGain<double>()` which computes `pow(10, dB / 20)`. No allocations, no locks.
**Minimal Example:** `var gain = Engine.getGainFactorForDecibels(-6.0);`

**Description:**
Converts a decibel value to a linear gain factor. Uses the standard formula `pow(10, dB / 20)`. A value of 0 dB returns 1.0, -6 dB returns approximately 0.5012, and the JUCE implementation returns 0.0 for values at or below the minimum threshold (typically -100 dB). This is the inverse of `Engine.getDecibelsForGainFactor()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| decibels | Number | yes | The decibel value to convert to a linear gain factor. | Values at or below -100 dB return 0.0. |

**Cross References:**
- `$API.Engine.getDecibelsForGainFactor$`

## getGlobalPitchFactor

**Signature:** `double getGlobalPitchFactor()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a `double` member variable (`globalPitchFactor`) and computes `log2(x) * 12.0`. No allocations, no locks.
**Minimal Example:** `var pitch = Engine.getGlobalPitchFactor();`

**Description:**
Returns the current global pitch factor in semitones. The internal storage is a linear pitch ratio (set via `setGlobalPitchFactor` which converts semitones to ratio using `pow(2, st/12)`), and this getter converts it back to semitones using `log2(ratio) * 12.0`. The value is clamped to the range -12..12 semitones by the setter. Returns 0.0 when no pitch shift has been applied.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.setGlobalPitchFactor$`

## getGlobalRoutingManager

**Signature:** `var getGlobalRoutingManager()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new `GlobalRoutingManagerReference` object on the heap, which registers itself as an OSC listener and allocates internal callback storage.
**Minimal Example:** `var rm = Engine.getGlobalRoutingManager();`

**Description:**
Returns a `GlobalRoutingManager` scripting reference that provides access to global cables and OSC communication. Each call creates a new wrapper object, but all wrappers reference the same underlying singleton routing manager. The returned object can be used to get cable references (`getCable`), connect to OSC (`connectToOSC`), register OSC callbacks (`addOSCCallback`), send OSC messages (`sendOSCMessage`), and manage event data slots (`setEventData`/`getEventData`). Store the reference in a `const var` -- creating multiple wrappers is harmless but wasteful.

**Parameters:**
None.

**Cross References:**
- `$API.GlobalRoutingManager.getCable$`
- `$API.GlobalRoutingManager.connectToOSC$`

**Example:**
```javascript:global-routing-cable
// Title: Get a global cable and set its value
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("MyCable");
cable.setValue(0.75);
```
```json:testMetadata:global-routing-cable
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cable.getValueNormalised()", "value": 0.75}
  ]
}
```

## getHostBpm

**Signature:** `double getHostBpm()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads an `atomic<double>` (`bpm`) via `MainController::getBpm()`. The load is lock-free and allocation-free. A conditional check returns 120.0 if the stored value is <= 0.
**Minimal Example:** `var bpm = Engine.getHostBpm();`

**Description:**
Returns the current BPM (beats per minute) of the host or the manually set BPM override. If the BPM has been overridden via `Engine.setHostBpm()` with a positive value, that value is returned. If no override is set (i.e., the internal value is <= 0), the method returns a default of 120.0 BPM. In a DAW plugin context, the host-reported BPM is used unless overridden. In standalone mode, the default of 120.0 applies unless `setHostBpm()` has been called.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.setHostBpm$`
- `$API.Engine.createTransportHandler$`
- `$API.Engine.getMilliSecondsForTempo$`

## getLatencySamples

**Signature:** `int getLatencySamples()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads the latency value from the underlying `AudioProcessor` via `getLatencySamples()`. This is a simple getter returning a cached integer -- no allocations, no locks.
**Minimal Example:** `var latency = Engine.getLatencySamples();`

**Description:**
Returns the plugin's latency in samples as reported to the host. The default value is 0. This value is set via `Engine.setLatencySamples()` and corresponds to the JUCE `AudioProcessor::getLatencySamples()` call, which the host uses for delay compensation.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.setLatencySamples$`

## getLorisManager

**Signature:** `var getLorisManager()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new `ScriptLorisManager` object on each call (heap allocation).
**Minimal Example:** `var lm = Engine.getLorisManager();`

**Description:**
Returns a reference to the Loris spectral analysis manager. Creates a new `ScriptLorisManager` wrapper object on each call. Requires the `HISE_INCLUDE_LORIS` preprocessor flag to be enabled at compile time. If Loris support is not compiled in, returns `undefined` without error.

**Parameters:**
None.

**Pitfalls:**
- Returns `undefined` silently when `HISE_INCLUDE_LORIS` is not enabled at compile time. There is no error message or console warning -- the caller must check the return value.

**Cross References:**
None.

## getMacroName

**Signature:** `String getMacroName(int index)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a `String` from the macro control data, which involves atomic reference counting.
**Minimal Example:** `var name = Engine.getMacroName(1);`

**Description:**
Returns the name of the macro at the given 1-based index (1 through 8). The macro names are defined via `Engine.setFrontendMacros()`. If the index is out of range, a script error is reported and the string `"Undefined"` is returned.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | The 1-based macro index. | 1-8 |

**Pitfalls:**
- Uses 1-based indexing (1-8), not 0-based. Passing 0 reports "Illegal Macro Index".

**Cross References:**
- `$API.Engine.setFrontendMacros$`
- `$API.Engine.createMacroHandler$`

## getMasterPeakLevel

**Signature:** `double getMasterPeakLevel(int channel)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a cached display value (`outL`/`outR`) from the main synth chain. No allocations, no locks -- just a struct member read.
**Minimal Example:** `var peakL = Engine.getMasterPeakLevel(0);`

**Description:**
Returns the current peak volume level (0.0 to 1.0) for the given output channel. Pass 0 for the left channel and 1 for the right channel. Currently only stereo output is supported -- any channel value other than 0 returns the right channel level. The returned value comes from the main synth chain's display values, which are updated each audio buffer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | The output channel index. | 0 = left, 1 = right |

**Pitfalls:**
- Any channel value other than 0 silently returns the right channel level. There is no bounds checking or error for invalid channel indices (e.g., passing 5 returns the right channel level without warning).

**Cross References:**
- `$API.Engine.getNumPluginChannels$`

## getMemoryUsage

**Signature:** `double getMemoryUsage()`
**Return Type:** `Double`
**Call Scope:** unsafe
**Call Scope Note:** Iterates over all expansions calling `getMemoryUsageForAllSamples()` on each pool. The expansion handler access and pool iteration involve potential lock acquisitions and heap-allocated data traversal.
**Minimal Example:** `var mem = Engine.getMemoryUsage();`

**Description:**
Returns the total memory usage of all loaded samples in megabytes (MB). This includes samples from the main sample pool and all expansion sample pools. The value is computed by summing the byte counts from each pool and dividing by 1024*1024 to convert to MB.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getCpuUsage$`
- `$API.Engine.getNumVoices$`

## getMidiNoteFromName

**Signature:** `int getMidiNoteFromName(String midiNoteName)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Accepts a `String` parameter and performs 128 string comparisons in the worst case.
**Minimal Example:** `var noteNum = Engine.getMidiNoteFromName("C3");`

**Description:**
Converts a MIDI note name string (e.g., `"C3"` for middle C) to its corresponding MIDI note number (0-127). The method iterates through all 128 MIDI notes, calling `getMidiNoteName()` on each and comparing the result to the input string. Returns -1 if no match is found. The note naming convention uses C3 as middle C (MIDI note 60).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| midiNoteName | String | no | The MIDI note name to look up (e.g., `"C3"`, `"F#4"`). | Must match JUCE's `MidiMessage::getMidiNoteName()` output format. |

**Pitfalls:**
- Uses a brute-force linear search (O(128) string comparisons). This is negligible for single calls but should not be used in tight loops on the audio thread.
- Returns -1 for unrecognized names without any console warning. The caller must check for -1.

**Cross References:**
- `$API.Engine.getMidiNoteName$`
- `$API.Engine.getFrequencyForMidiNoteNumber$`

## getMidiNoteName

**Signature:** `String getMidiNoteName(int midiNumber)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a `String` constructed by JUCE's `MidiMessage::getMidiNoteName()`.
**Minimal Example:** `var name = Engine.getMidiNoteName(60);`

**Description:**
Converts a MIDI note number (0-127) to its note name string. Middle C (note 60) returns `"C3"`. Uses JUCE's `MidiMessage::getMidiNoteName()` with sharps enabled, octave number displayed, and middle C octave set to 3. The implementation is inline in the header: `MidiMessage::getMidiNoteName(midiNumber, true, true, 3)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| midiNumber | Integer | no | The MIDI note number to convert. | 0-127 |

**Cross References:**
- `$API.Engine.getMidiNoteFromName$`
- `$API.Engine.getFrequencyForMidiNoteNumber$`

## getMilliSecondsForQuarterBeats

**Signature:** `double getMilliSecondsForQuarterBeats(double quarterBeats)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure math chain: calls `getSamplesForQuarterBeats()` (which calls `getSamplesForQuarterBeatsWithTempo()` with `getHostBpm()`) then `getMilliSecondsForSamples()`. All are inline arithmetic -- no allocations, no locks. `getHostBpm()` reads an atomic.
**Minimal Example:** `var ms = Engine.getMilliSecondsForQuarterBeats(1.0);`

**Description:**
Converts a duration expressed in quarter beats to milliseconds using the current host BPM and sample rate. Internally delegates to `getSamplesForQuarterBeats()` (which uses `TempoSyncer::getTempoInSamples()` with the current BPM) and then `getMilliSecondsForSamples()` to convert the sample count to milliseconds. At 120 BPM, one quarter beat equals 500 ms. Use `getMilliSecondsForQuarterBeatsWithTempo()` if you need to specify an explicit BPM instead of the current host tempo.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| quarterBeats | Number | yes | The duration in quarter beats to convert. | Any positive number. |

**Cross References:**
- `$API.Engine.getMilliSecondsForQuarterBeatsWithTempo$`
- `$API.Engine.getSamplesForQuarterBeats$`
- `$API.Engine.getQuarterBeatsForMilliSeconds$`
- `$API.Engine.getHostBpm$`

## getMilliSecondsForQuarterBeatsWithTempo

**Signature:** `double getMilliSecondsForQuarterBeatsWithTempo(double quarterBeats, double bpm)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure math chain: delegates to `getSamplesForQuarterBeatsWithTempo()` (which calls `TempoSyncer::getTempoInSamples()`) then `getMilliSecondsForSamples()`. All inline arithmetic -- no allocations, no locks.
**Minimal Example:** `var ms = Engine.getMilliSecondsForQuarterBeatsWithTempo(1.0, 140.0);`

**Description:**
Converts a duration expressed in quarter beats to milliseconds using an explicitly provided BPM value and the current sample rate. Internally computes the sample count via `getSamplesForQuarterBeatsWithTempo()` (which multiplies `TempoSyncer::getTempoInSamples()` for a quarter note by the beat count) and then converts samples to milliseconds. This is the explicit-tempo variant of `getMilliSecondsForQuarterBeats()`, useful when calculating timing for a BPM other than the current host tempo.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| quarterBeats | Double | no | The duration in quarter beats to convert. | Any positive number. |
| bpm | Double | no | The tempo in beats per minute to use for conversion. | Must be positive. |

**Cross References:**
- `$API.Engine.getMilliSecondsForQuarterBeats$`
- `$API.Engine.getSamplesForQuarterBeatsWithTempo$`
- `$API.Engine.getQuarterBeatsForMilliSecondsWithTempo$`

## getMilliSecondsForSamples

**Signature:** `double getMilliSecondsForSamples(double samples)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Inline arithmetic: `samples / getSampleRate() * 1000.0`. No allocations, no locks.
**Minimal Example:** `var ms = Engine.getMilliSecondsForSamples(44100.0);`

**Description:**
Converts a sample count to milliseconds using the current sample rate. The formula is `samples / sampleRate * 1000.0`. This is the inverse of `getSamplesForMilliSeconds()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| samples | Number | yes | The number of samples to convert. | Any non-negative number. |

**Cross References:**
- `$API.Engine.getSamplesForMilliSeconds$`
- `$API.Engine.getSampleRate$`

## getMilliSecondsForTempo

**Signature:** `double getMilliSecondsForTempo(int tempoIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Delegates to `TempoSyncer::getTempoInMilliSeconds()` which is pure arithmetic. `getHostBpm()` reads an atomic.
**Minimal Example:** `var ms = Engine.getMilliSecondsForTempo(5);`

**Description:**
Returns the duration in milliseconds for the given tempo subdivision index at the current host BPM. The `tempoIndex` maps to the `TempoSyncer::Tempo` enum: 0 = 1/1 (whole note), 5 = 1/4 (quarter note), 8 = 1/8 (eighth note), etc. Use `Engine.getTempoName()` to get the human-readable name for a given index. The index values correspond to the tempo sync options available on HISE slider components in "TempoSync" mode.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tempoIndex | Number | yes | Index into the TempoSyncer::Tempo enum. | 0-18 (standard), or 0-23 with `HISE_USE_EXTENDED_TEMPO_VALUES`. |

**Cross References:**
- `$API.Engine.getTempoName$`
- `$API.Engine.getHostBpm$`
- `$API.Engine.getSamplesForMilliSeconds$`

## getName

**Signature:** `String getName()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Backend path accesses the settings object; frontend path calls `FrontendHandler::getProjectName()`.
**Minimal Example:** `var name = Engine.getName();`

**Description:**
Returns the product name as defined in the project settings. In the HISE IDE (backend), reads from `HiseSettings::Project::Name`. In exported plugins (frontend), returns the value baked by `FrontendHandler::getProjectName()`. This returns the project name, not the HISE engine name. Use `Engine.getVersion()` for the project version string, or `Engine.getProjectInfo()` for the full project metadata object.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getVersion$`
- `$API.Engine.getProjectInfo$`

## getNumPluginChannels

**Signature:** `int getNumPluginChannels()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Returns the compile-time constant `HISE_NUM_PLUGIN_CHANNELS`. No computation, no allocations.
**Minimal Example:** `var ch = Engine.getNumPluginChannels();`

**Description:**
Returns the number of output channels configured for the plugin, which is the compile-time constant `HISE_NUM_PLUGIN_CHANNELS` (default 2 for stereo). This value is set at export time and cannot be changed at runtime. For multi-output plugins, this value is derived from the master container's routing matrix channel count.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getBufferSize$`
- `$API.Engine.getSampleRate$`

## getNumVoices

**Signature:** `int getNumVoices()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads `activeVoices.size()` from each synth in the chain. No allocations, no locks. Iterates child synths but the count is architecturally fixed, not user-sized.
**Minimal Example:** `var voices = Engine.getNumVoices();`

**Description:**
Returns the total number of currently active (sounding) voices across all synthesisers in the module tree. Traverses the main synth chain and sums the active voice count from each child synth. Useful for CPU monitoring or voice-count-aware UI displays. Note that this returns currently *active* voices, not the maximum polyphony setting.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getCpuUsage$`
- `$API.Engine.getMemoryUsage$`

## getOS

**Signature:** `String getOS()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a String constructed from a compile-time string literal.
**Minimal Example:** `var os = Engine.getOS();`

**Description:**
Returns the current operating system as a short string identifier. The value is determined at compile time via preprocessor guards: `"WIN"` on Windows, `"OSX"` on macOS, `"LINUX"` on Linux. Useful for platform-specific UI adjustments or file path handling.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getDeviceType$`
- `$API.Engine.getSystemStats$`
- `$API.Engine.isPlugin$`

## getPitchRatioFromSemitones

**Signature:** `double getPitchRatioFromSemitones(double semiTones)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure inline arithmetic: `pow(2.0, semiTones / 12.0)`. No allocations, no locks.
**Minimal Example:** `var ratio = Engine.getPitchRatioFromSemitones(7.0);`

**Description:**
Converts a semitone offset to a pitch ratio using the formula `2^(semitones/12)`. A value of 0 returns 1.0 (no pitch change), 12 returns 2.0 (one octave up), -12 returns 0.5 (one octave down), and 7 returns approximately 1.498 (a perfect fifth). The input is not clamped -- any semitone value is accepted. This is the inverse of `getSemitonesFromPitchRatio()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| semiTones | Number | yes | The semitone offset to convert. | Any number. Typical range: -12 to 12. |

**Cross References:**
- `$API.Engine.getSemitonesFromPitchRatio$`
- `$API.Engine.getFrequencyForMidiNoteNumber$`

## getPlayHead

**Signature:** `DynamicObject * getPlayHead()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Returns a heap-allocated `DynamicObject` pointer. The object itself is persistent (owned by MainController), but accessing it involves pointer dereference through MainController.
**Minimal Example:** `var ph = Engine.getPlayHead();`

**Description:**
Returns a reference to the host transport information object. This object is intended to contain properties such as `bpm`, `timeSigNumerator`, `timeSigDenominator`, `timeInSamples`, `timeInSeconds`, `ppqPosition`, `ppqPositionOfLastBarStart`, `isPlaying`, `isRecording`, `ppqLoopStart`, `ppqLoopEnd`, and `isLooping`. However, the code that populates these properties in `MainController::setHostBpm()` is entirely commented out, so the returned object is empty. Use `Engine.createTransportHandler()` for reliable host transport data, or `Engine.getHostBpm()` for the current BPM.

**Parameters:**
None.

**Pitfalls:**
- [BUG] The returned object is empty because the property-population code in `MainController::setHostBpm()` (lines 1707-1720) is entirely commented out. All properties (`bpm`, `isPlaying`, `ppqPosition`, etc.) are undefined on the returned object. Use `Engine.createTransportHandler()` or `Engine.getHostBpm()` instead.

**Cross References:**
- `$API.Engine.createTransportHandler$`
- `$API.Engine.getHostBpm$`

## getPreloadMessage

**Signature:** `String getPreloadMessage()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a `String` from `SampleManager::getPreloadMessage()`.
**Minimal Example:** `var msg = Engine.getPreloadMessage();`

**Description:**
Returns the current sample preload status message. During sample loading, this contains a human-readable description of what is being loaded (e.g., the name of the sample map or audio file currently being processed). Returns an empty string when no preload operation is active. Typically polled from a timer callback to display loading progress to the user.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getPreloadProgress$`
- `$API.Engine.setPreloadMessage$`

## getPreloadProgress

**Signature:** `double getPreloadProgress()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a numeric value from `SampleManager::getPreloadProgress()`. No allocations, no locks, no string involvement.
**Minimal Example:** `var progress = Engine.getPreloadProgress();`

**Description:**
Returns the current sample preload progress as a value from 0.0 to 1.0. Returns 0.0 when no preload operation is active or when preloading has not yet started, and approaches 1.0 as preloading completes. Typically polled from a timer callback alongside `Engine.getPreloadMessage()` to display a loading bar or spinner to the user.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getPreloadMessage$`
- `$API.Engine.setPreloadMessage$`

## getProjectInfo

**Signature:** `var getProjectInfo()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new `DynamicObject` on the heap and constructs multiple `String` properties via `GET_HISE_SETTING` (backend) or `FrontendHandler` static methods (frontend).
**Minimal Example:** `var info = Engine.getProjectInfo();`

**Description:**
Returns a JSON object containing project and company metadata from the project settings. The returned object has these properties:

| Property | Description |
|----------|-------------|
| `Company` | Company name from project settings |
| `CompanyURL` | Company website URL |
| `CompanyCopyright` | Copyright string |
| `ProjectName` | The project/product name |
| `ProjectVersion` | Version string (e.g., "1.0.0") |
| `EncryptionKey` | The project's expansion encryption key |
| `HISEBuild` | HISE version string used to build the project |
| `BuildDate` | Compilation date string |
| `LicensedEmail` | Licensed user's email (requires `USE_BACKEND` or `USE_COPY_PROTECTION`; empty string otherwise) |

In the HISE IDE (backend), values are read from `HiseSettings`. In compiled plugins (frontend), values are read from the baked `FrontendHandler` data. The `LicensedEmail` property is only populated when `USE_BACKEND` or `USE_COPY_PROTECTION` is enabled -- otherwise it is an empty string.

**Parameters:**
None.

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Company | String | Company name from project settings |
| CompanyURL | String | Company website URL |
| CompanyCopyright | String | Copyright string |
| ProjectName | String | Product/project name |
| ProjectVersion | String | Version string |
| EncryptionKey | String | Expansion encryption key |
| HISEBuild | String | HISE build version |
| BuildDate | String | Compilation date |
| LicensedEmail | String | Licensed user email (empty if copy protection not enabled) |

**Cross References:**
- `$API.Engine.getName$`
- `$API.Engine.getVersion$`

**Example:**
```javascript:project-info-display
// Title: Displaying project info in the console
var info = Engine.getProjectInfo();
Console.print(info.ProjectName + " v" + info.ProjectVersion);
Console.print("Built with HISE " + info.HISEBuild);
```
```json:testMetadata:project-info-display
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof info.ProjectName", "value": "string"},
    {"type": "REPL", "expression": "info.ProjectName.length > 0", "value": true},
    {"type": "REPL", "expression": "typeof info.HISEBuild", "value": "string"},
    {"type": "REPL", "expression": "info.HISEBuild.length > 0", "value": true}
  ]
}
```

## getQuarterBeatsForMilliSeconds

**Signature:** `double getQuarterBeatsForMilliSeconds(double milliSeconds)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure arithmetic chain: converts milliseconds to samples via `getSamplesForMilliSeconds()`, then delegates to `getQuarterBeatsForSamples()` which calls `getQuarterBeatsForSamplesWithTempo()`. All operations are inline multiplication/division with no allocations or locks.
**Minimal Example:** `var beats = Engine.getQuarterBeatsForMilliSeconds(500.0);`

**Description:**
Converts a duration in milliseconds to quarter beats using the current host BPM and sample rate. Internally converts milliseconds to samples first (via the current sample rate), then converts samples to quarter beats using the current BPM. At 120 BPM, 500 ms equals 1.0 quarter beat. This is the inverse of `Engine.getMilliSecondsForQuarterBeats()`. Uses the BPM value from `Engine.getHostBpm()` -- if no host BPM is set and no override is active, `TempoSyncer` defaults to 120 BPM.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| milliSeconds | Number | yes | Duration in milliseconds to convert. | Any positive number. |

**Cross References:**
- `$API.Engine.getQuarterBeatsForMilliSecondsWithTempo$`
- `$API.Engine.getMilliSecondsForQuarterBeats$`
- `$API.Engine.getQuarterBeatsForSamples$`
- `$API.Engine.getHostBpm$`

## getQuarterBeatsForMilliSecondsWithTempo

**Signature:** `double getQuarterBeatsForMilliSecondsWithTempo(double milliSeconds, double bpm)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure arithmetic chain: converts milliseconds to samples, then delegates to `getQuarterBeatsForSamplesWithTempo()`. All inline multiplication/division with no allocations or locks.
**Minimal Example:** `var beats = Engine.getQuarterBeatsForMilliSecondsWithTempo(500.0, 120.0);`

**Description:**
Converts a duration in milliseconds to quarter beats using an explicit BPM value and the current sample rate. Internally converts milliseconds to samples first, then divides by the number of samples per quarter beat at the given tempo. At 120 BPM, 500 ms equals 1.0 quarter beat. If `bpm` is 0.0, `TempoSyncer` internally defaults to 120 BPM. This is the explicit-tempo variant of `Engine.getQuarterBeatsForMilliSeconds()`, useful when computing conversions at a tempo different from the current host BPM.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| milliSeconds | Number | no | Duration in milliseconds to convert. | Any positive number. |
| bpm | Number | no | Tempo in beats per minute. | Positive number. 0.0 defaults to 120 internally. |

**Cross References:**
- `$API.Engine.getQuarterBeatsForMilliSeconds$`
- `$API.Engine.getMilliSecondsForQuarterBeatsWithTempo$`
- `$API.Engine.getQuarterBeatsForSamplesWithTempo$`

## getQuarterBeatsForSamples

**Signature:** `double getQuarterBeatsForSamples(double samples)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure arithmetic: delegates to `getQuarterBeatsForSamplesWithTempo(samples, getHostBpm())`. All inline multiplication/division via `TempoSyncer::getTempoInSamples()` with no allocations or locks.
**Minimal Example:** `var beats = Engine.getQuarterBeatsForSamples(22050.0);`

**Description:**
Converts a sample count to quarter beats using the current host BPM and sample rate. Delegates to `getQuarterBeatsForSamplesWithTempo()` with the current BPM from `Engine.getHostBpm()`. The formula is: `samples / samplesPerQuarterBeat`, where `samplesPerQuarterBeat = (60.0 / bpm) * sampleRate`. This is the inverse of `Engine.getSamplesForQuarterBeats()`. If no host BPM is set and no override is active, `TempoSyncer` defaults to 120 BPM.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| samples | Number | yes | Number of audio samples to convert. | Any positive number. |

**Cross References:**
- `$API.Engine.getQuarterBeatsForSamplesWithTempo$`
- `$API.Engine.getSamplesForQuarterBeats$`
- `$API.Engine.getQuarterBeatsForMilliSeconds$`
- `$API.Engine.getHostBpm$`

## getQuarterBeatsForSamplesWithTempo

**Signature:** `double getQuarterBeatsForSamplesWithTempo(double samples, double bpm)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Pure arithmetic: `samples / TempoSyncer::getTempoInSamples(bpm, sampleRate, Quarter)`. No allocations, no locks.
**Minimal Example:** `var beats = Engine.getQuarterBeatsForSamplesWithTempo(22050.0, 120.0);`

**Description:**
Converts a sample count to quarter beats using an explicit BPM value and the current sample rate. Computes `samples / samplesPerQuarterBeat`, where `samplesPerQuarterBeat` is derived from `TempoSyncer::getTempoInSamples(bpm, sampleRate, Quarter)`. At 120 BPM and 44100 Hz sample rate, one quarter beat equals 22050 samples. If `bpm` is 0.0, `TempoSyncer` internally defaults to 120 BPM. This is the explicit-tempo variant of `Engine.getQuarterBeatsForSamples()`, useful for computing conversions at a tempo different from the current host BPM. This is the inverse of `Engine.getSamplesForQuarterBeatsWithTempo()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| samples | Number | no | Number of audio samples to convert. | Any positive number. |
| bpm | Number | no | Tempo in beats per minute. | Positive number. 0.0 defaults to 120 internally. |

**Cross References:**
- `$API.Engine.getQuarterBeatsForSamples$`
- `$API.Engine.getSamplesForQuarterBeatsWithTempo$`
- `$API.Engine.getQuarterBeatsForMilliSecondsWithTempo$`

## getRegexMatches

**Signature:** `Array getRegexMatches(String stringToMatch, String regex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Constructs std::regex and std::string objects on the heap; iterates with std::regex_search which may allocate internally.
**Minimal Example:** `var matches = Engine.getRegexMatches("note_C3_vel127", "([A-G]\\d)");`

**Description:**
Returns an array of all regex matches found in the input string. Uses C++ `std::regex` with `std::regex_search` in a loop, collecting all matches including capture groups. Each iteration appends all sub-matches (the full match plus any capture groups) to the result array. A safety limit of 100,000 iterations prevents runaway matching. If the regex is invalid, a console error is printed and `undefined` is returned instead of an array.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| stringToMatch | String | no | The input string to search within. | Any string. |
| regex | String | no | A C++ std::regex pattern string. | Must be a valid ECMAScript regex (std::regex default grammar). |

**Pitfalls:**
- Invalid regex patterns return `undefined` instead of an empty array. Check `isDefined()` on the return value if user-provided patterns are possible.
- Each loop iteration appends ALL sub-matches (full match + capture groups), not just the full match. A pattern with N capture groups produces N+1 entries per match occurrence.

**Cross References:**
- `$API.Engine.matchesRegex$`

## getSampleFilesFromDirectory

**Signature:** `Array getSampleFilesFromDirectory(String relativePathFromSampleFolder, Integer recursive)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Performs filesystem I/O (directory listing), constructs String and Array objects on the heap.
**Minimal Example:** `var files = Engine.getSampleFilesFromDirectory("Kicks", true);`

**Description:**
Iterates the given subdirectory of the project's Samples folder and returns an array of pool reference strings for all audio files found. Only `.wav`, `.aif`, and `.aiff` files are included (case-insensitive check via both upper and lower case extensions). Hidden files and files whose names start with `.` are excluded. This method is backend-only -- in compiled plugins it returns an empty array silently.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| relativePathFromSampleFolder | String | no | Relative path from the project's Samples directory to the subdirectory to scan. | Must point to an existing directory; a script error is thrown if not found. |
| recursive | Integer | no | Whether to search subdirectories recursively. | Boolean: 0 or 1. |

**Pitfalls:**
- Returns an empty array in compiled (frontend) plugins with no error or warning. Code relying on this method will silently produce no results in exported plugins.

**Cross References:**
- `$API.Engine.loadAudioFilesIntoPool$`

## getSampleRate

**Signature:** `Double getSampleRate()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var sr = Engine.getSampleRate();`

**Description:**
Returns the current audio sample rate in Hz as reported by the main synthesiser chain (e.g. 44100.0, 48000.0, 96000.0). The value reflects the actual sample rate configured by the host or audio driver. This is the same sample rate used internally by all unit conversion methods (`getSamplesForMilliSeconds`, `getMilliSecondsForSamples`, etc.).

**Parameters:**

(None.)

**Cross References:**
- `$API.Engine.getSamplesForMilliSeconds$`
- `$API.Engine.getMilliSecondsForSamples$`
- `$API.Engine.getBufferSize$`

## getSamplesForMilliSeconds

**Signature:** `Double getSamplesForMilliSeconds(Number milliSeconds)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var samples = Engine.getSamplesForMilliSeconds(100.0);`

**Description:**
Converts a time value in milliseconds to the equivalent number of audio samples at the current sample rate. The formula is `(milliSeconds / 1000.0) * getSampleRate()`. Returns a floating-point value; use `Math.round()` if an integer sample count is needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| milliSeconds | Number | yes | Time in milliseconds to convert. | Any number. Negative values produce negative results. |

**Cross References:**
- `$API.Engine.getMilliSecondsForSamples$`
- `$API.Engine.getSampleRate$`

## getSamplesForQuarterBeats

**Signature:** `Double getSamplesForQuarterBeats(Number quarterBeats)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var samples = Engine.getSamplesForQuarterBeats(4.0);`

**Description:**
Converts a duration in quarter beats to the equivalent number of audio samples using the current host BPM. Delegates to `getSamplesForQuarterBeatsWithTempo(quarterBeats, getHostBpm())`. The result depends on the current sample rate and BPM -- if either changes, the same input produces a different output.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| quarterBeats | Number | yes | Duration in quarter beats (1.0 = one quarter note). | Any positive number. |

**Cross References:**
- `$API.Engine.getSamplesForQuarterBeatsWithTempo$`
- `$API.Engine.getQuarterBeatsForSamples$`
- `$API.Engine.getMilliSecondsForQuarterBeats$`

## getSamplesForQuarterBeatsWithTempo

**Signature:** `Double getSamplesForQuarterBeatsWithTempo(Number quarterBeats, Number bpm)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var samples = Engine.getSamplesForQuarterBeatsWithTempo(4.0, 140.0);`

**Description:**
Converts a duration in quarter beats to the equivalent number of audio samples using an explicitly provided BPM value instead of the current host BPM. Internally computes `TempoSyncer::getTempoInSamples(bpm, getSampleRate(), TempoSyncer::Quarter)` and multiplies by `quarterBeats`. Useful for offline calculations or when a tempo different from the host tempo is needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| quarterBeats | Number | no | Duration in quarter beats (1.0 = one quarter note). | Any positive number. |
| bpm | Number | no | Tempo in beats per minute to use for the conversion. | Positive number. |

**Cross References:**
- `$API.Engine.getSamplesForQuarterBeats$`
- `$API.Engine.getQuarterBeatsForSamplesWithTempo$`
- `$API.Engine.getMilliSecondsForQuarterBeatsWithTempo$`

## getSemitonesFromPitchRatio

**Signature:** `Double getSemitonesFromPitchRatio(Number pitchRatio)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var semitones = Engine.getSemitonesFromPitchRatio(2.0);`

**Description:**
Converts a pitch ratio to a value using the formula `1200.0 * log2(pitchRatio)`. Despite the method name and documentation suggesting the result is in semitones, the formula actually returns cents (1/100th of a semitone). For a pitch ratio of 2.0 (one octave up), the method returns 1200.0 rather than 12.0. The inverse method `getPitchRatioFromSemitones` correctly uses semitones (`pow(2.0, semiTones / 12.0)`), so these two methods are not true inverses of each other.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pitchRatio | Number | yes | Pitch ratio to convert. 1.0 = unity, 2.0 = one octave up, 0.5 = one octave down. | Any positive number. |

**Pitfalls:**
- [BUG] Returns cents (1200 per octave) instead of semitones (12 per octave) despite the method name. The formula `1200.0 * log2(ratio)` should be `12.0 * log2(ratio)` to return semitones. Divide the result by 100 to get actual semitones, or use `getPitchRatioFromSemitones` as the authoritative direction and compute the inverse manually.

**Cross References:**
- `$API.Engine.getPitchRatioFromSemitones$`

## getSettingsWindowObject

**Disabled:** deprecated
**Disabled Reason:** Hard deprecated. Calls `reportScriptError("Deprecated")` and returns undefined. The Settings class provides the replacement API for audio device and application configuration.

## getStringWidth

**Signature:** `float getStringWidth(String text, String fontName, float fontSize, float fontSpacing)`
**Return Type:** `Double`
**Call Scope:** unsafe
**Call Scope Note:** Iterates over custom typeface list (string comparisons, potential font metrics computation involving heap-allocated glyph data).
**Minimal Example:** `var w = Engine.getStringWidth("Hello", "Arial", 16.0, 0.0);`

**Description:**
Returns the pixel width of the given text string when rendered with the specified font properties. Looks up the font by name among custom typefaces loaded via `Engine.loadFontAs()`. If no custom typeface matches the given `fontName`, falls back to the default embedded font. This is useful for calculating text layout widths in custom paint routines or LAF callbacks where text must be positioned precisely.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | The text string to measure. | -- |
| fontName | String | no | The registered font name (as passed to `loadFontAs`) or the typeface's internal name. | Must be a loaded font name or falls back to default. |
| fontSize | Double | no | The font size in pixels. | Positive value. |
| fontSpacing | Double | no | The kerning/character spacing factor. 0.0 = default spacing. | -- |

**Cross References:**
- `$API.Engine.loadFontAs$`
- `$API.Engine.setGlobalFont$`
- `$API.Graphics.getStringWidth$`

## getSystemStats

**Signature:** `var getSystemStats()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new `DynamicObject` on the heap and constructs multiple String properties from JUCE `SystemStats` calls.
**Minimal Example:** `var stats = Engine.getSystemStats();`

**Description:**
Returns a JSON object containing detailed information about the current system hardware and OS configuration. Each call allocates a new object. Useful for telemetry, system requirement checks, or adapting UI based on system capabilities (e.g., checking `isDarkMode` to match the OS theme).

**Parameters:**
None.

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| OperatingSystemName | String | Full OS name (e.g., "Windows 10", "macOS 14.0"). |
| OperatingSystem64Bit | Integer | 1 if the OS is 64-bit, 0 otherwise. |
| LogonName | String | Current user's logon name. |
| FullUserName | String | Current user's full display name. |
| ComputerName | String | Computer/hostname. |
| UserLanguage | String | ISO language code (e.g., "en"). |
| UserRegion | String | ISO region code (e.g., "US"). |
| DisplayLanguage | String | OS display language. |
| NumCpus | Integer | Number of logical CPU cores. |
| NumPhysicalCpus | Integer | Number of physical CPU cores. |
| CpuSpeedInMegahertz | Integer | CPU clock speed in MHz. |
| CpuVendor | String | CPU vendor string (e.g., "GenuineIntel", "AuthenticAMD"). |
| CpuModel | String | CPU model string. |
| MemorySizeInMegabytes | Integer | Total system RAM in MB. |
| isDarkMode | Integer | 1 if the OS dark mode is active, 0 otherwise. |

**Cross References:**
- `$API.Engine.getOS$`
- `$API.Engine.getDeviceType$`
- `$API.Engine.getDeviceResolution$`

**Example:**
```javascript:system-stats-dark-mode
// Title: Adapt UI theme based on OS dark mode setting
var stats = Engine.getSystemStats();
var useDarkTheme = stats.isDarkMode;
Console.print("Dark mode: " + useDarkTheme);
Console.print("CPU: " + stats.CpuModel);
Console.print("RAM: " + stats.MemorySizeInMegabytes + " MB");
```
```json:testMetadata:system-stats-dark-mode
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof stats.CpuModel", "value": "string"},
    {"type": "REPL", "expression": "stats.MemorySizeInMegabytes > 0", "value": true},
    {"type": "REPL", "expression": "stats.isDarkMode == 0 || stats.isDarkMode == 1", "value": true},
    {"type": "REPL", "expression": "stats.NumCpus > 0", "value": true}
  ]
}
```

## getSystemTime

**Signature:** `String getSystemTime(bool includeDividerCharacters)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a newly constructed String via JUCE `Time::toISO8601()`.
**Minimal Example:** `var t = Engine.getSystemTime(true);`

**Description:**
Returns the current system date and time as an ISO-8601 formatted string using the local timezone. When `includeDividerCharacters` is `true`, the output includes hyphens and colons (e.g., `"2025-03-19T14:30:00+01:00"`). When `false`, dividers are omitted (e.g., `"20250319T143000+0100"`). Delegates directly to JUCE's `Time::getCurrentTime().toISO8601()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| includeDividerCharacters | Integer | no | Whether to include `-`, `:` divider characters in the output. Pass `true` for standard readable format, `false` for compact format. | -- |

**Cross References:**
None.

## getTempoName

**Signature:** `String getTempoName(int tempoIndex)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Returns a String from a static array lookup.
**Minimal Example:** `var name = Engine.getTempoName(5);`

**Description:**
Returns the human-readable tempo name string for the given index. The index maps to the `TempoSyncer::Tempo` enum. Returns `"Invalid"` if the index is out of range. This is the display counterpart to `getMilliSecondsForTempo()` which returns the time value for the same index.

Standard tempo indices (without `HISE_USE_EXTENDED_TEMPO_VALUES`):

| Index | Name |
|-------|------|
| 0 | 1/1 |
| 1 | 1/2D |
| 2 | 1/2 |
| 3 | 1/2T |
| 4 | 1/4D |
| 5 | 1/4 |
| 6 | 1/4T |
| 7 | 1/8D |
| 8 | 1/8 |
| 9 | 1/8T |
| 10 | 1/16D |
| 11 | 1/16 |
| 12 | 1/16T |
| 13 | 1/32D |
| 14 | 1/32 |
| 15 | 1/32T |
| 16 | 1/64D |
| 17 | 1/64 |
| 18 | 1/64T |

With `HISE_USE_EXTENDED_TEMPO_VALUES` enabled, indices 0-4 are EightBar, SixBar, FourBar, ThreeBar, TwoBars, and all standard values shift by 5.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tempoIndex | Integer | no | Index into the TempoSyncer tempo enum. | 0-18 (standard) or 0-23 (extended). Out-of-range returns "Invalid". |

**Cross References:**
- `$API.Engine.getMilliSecondsForTempo$`

## getTextForValue

**Signature:** `String getTextForValue(double value, String converterMode)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Constructs and returns a formatted String.
**Minimal Example:** `var text = Engine.getTextForValue(440.0, "Frequency");`

**Description:**
Converts a numeric value to a formatted text string using one of the built-in converter modes. Creates a `ValueToTextConverter` for the given mode and calls its forward conversion. If the mode string does not match any known mode, the converter is inactive and returns the value as a plain rounded integer string (e.g., `"440"` instead of `"440 Hz"`).

Available modes and their formatting behavior:

| Mode | Example Input | Example Output | Notes |
|------|---------------|----------------|-------|
| `"Frequency"` | 440.0 | `"440 Hz"` | Values >= 1000 shown as kHz (e.g., `"1.5 kHz"`) |
| `"Time"` | 500.0 | `"500ms"` | Values > 1000 shown in seconds (e.g., `"1.5s"`) |
| `"TempoSync"` | 5.0 | `"1/4"` | Uses TempoSyncer index lookup (same indices as `getTempoName`) |
| `"Pan"` | 0.0 | `"C"` | Positive = R, negative = L (e.g., `"50R"`, `"50L"`) |
| `"NormalizedPercentage"` | 0.75 | `"75%"` | Multiplies by 100 and rounds |
| `"Decibel"` | -6.0 | `"-6.0 dB"` | Values below -120 shown as `"-INF"` |
| `"Semitones"` | 2.0 | `"+2 st"` | Includes sign prefix, fractional shown to 2 decimal places |

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Double | no | The numeric value to convert to text. | -- |
| converterMode | String | no | The converter mode name. | One of: `"Frequency"`, `"Time"`, `"TempoSync"`, `"Pan"`, `"NormalizedPercentage"`, `"Decibel"`, `"Semitones"`. |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"Frequency"` | Formats as Hz or kHz with automatic unit scaling at the 1000 Hz threshold. |
| `"Time"` | Formats as milliseconds or seconds with automatic unit scaling at 1000 ms. |
| `"TempoSync"` | Converts integer index to musical tempo notation (1/4, 1/8T, etc.). |
| `"Pan"` | Formats as stereo pan position with L/R suffix or C for center. |
| `"NormalizedPercentage"` | Converts 0.0-1.0 range to 0-100% display. |
| `"Decibel"` | Formats as dB with -INF for silence. |
| `"Semitones"` | Formats with +/- prefix and "st" suffix. |

**Pitfalls:**
- An invalid mode string silently falls back to plain integer conversion (e.g., `Engine.getTextForValue(440.0, "Hz")` returns `"440"` instead of `"440 Hz"`). No error or warning is produced for unrecognized modes.

**Cross References:**
- `$API.Engine.getValueForText$`
- `$API.Engine.getTempoName$`

## getUptime

**Signature:** `double getUptime()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a double from `MainController::getUptime()` (atomic/lock-free). The MIDI event timestamp path also only reads from the current event (no allocations, no locks).
**Minimal Example:** `var t = Engine.getUptime();`

**Description:**
Returns the engine uptime in seconds as a double. When called from a MIDI callback (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`) and a `HiseEvent` is currently active, the returned value includes the event's sub-buffer timestamp offset divided by the sample rate. This provides sample-accurate timing within the audio buffer rather than just the buffer start time. When called outside a MIDI callback (e.g., in `onInit`, paint routines, or timer callbacks), it returns the raw engine uptime without timestamp adjustment.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getSampleRate$`

## getUserPresetList

**Signature:** `var getUserPresetList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Performs file system I/O (recursive directory scan via `findChildFiles`), allocates String and Array objects on the heap.
**Minimal Example:** `var presets = Engine.getUserPresetList();`

**Description:**
Returns an array of all available user preset file paths as strings. Each entry is a relative path from the UserPresets root directory, without the `.preset` file extension, using forward slashes as directory separators (backslashes are normalized to `/`). The scan is recursive, so presets in subdirectories are included with their relative path (e.g., `"Bass/Deep Bass"`). In the HISE backend, scans the project's UserPresets subdirectory. In exported plugins, scans the user-writable preset directory from `FrontendHandler::getUserPresetDirectory()`.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.loadUserPreset$`
- `$API.Engine.saveUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`
- `$API.Engine.loadNextUserPreset$`
- `$API.Engine.loadPreviousUserPreset$`

## getValueForText

**Signature:** `double getValueForText(String text, String convertedMode)`
**Return Type:** `Double`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Parses a String parameter.
**Minimal Example:** `var val = Engine.getValueForText("440 Hz", "Frequency");`

**Description:**
Parses a formatted text string back to a numeric value using one of the built-in converter modes. This is the inverse of `getTextForValue()`. Creates a `ValueToTextConverter` for the given mode and calls its inverse conversion. If the mode string does not match any known mode, the converter is inactive and returns a plain `getDoubleValue()` parse of the text.

Parsing behavior per mode:

| Mode | Example Input | Example Output | Notes |
|------|---------------|----------------|-------|
| `"Frequency"` | `"1.5 kHz"` | 1500.0 | Recognizes `k` suffix for kHz |
| `"Time"` | `"1.5s"` | 1500.0 | Recognizes `s` (seconds, multiplies by 1000) and `ms` (milliseconds, direct) |
| `"TempoSync"` | `"1/4"` | 5.0 | Looks up the tempo name in the TempoSyncer name table |
| `"Pan"` | `"50L"` | -50.0 | `"C"` returns 0.0, `L` suffix negates, `R` is positive |
| `"NormalizedPercentage"` | `"75%"` | 0.75 | Divides by 100 |
| `"Decibel"` | `"-INF"` | -100.0 | Handles `"-INF"` specially, otherwise parses double |
| `"Semitones"` | `"+2 st"` | 2.0 | Direct double parse (ignores suffix) |

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| text | String | no | The formatted text string to parse. | Should match the format produced by the corresponding `getTextForValue` mode. |
| convertedMode | String | no | The converter mode name. | One of: `"Frequency"`, `"Time"`, `"TempoSync"`, `"Pan"`, `"NormalizedPercentage"`, `"Decibel"`, `"Semitones"`. |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| `"Frequency"` | Parses Hz or kHz text back to a frequency in Hz. |
| `"Time"` | Parses seconds or milliseconds text back to a value in milliseconds. |
| `"TempoSync"` | Converts tempo name string (e.g., "1/4") back to its TempoSyncer index. |
| `"Pan"` | Parses pan position text back to a signed numeric value. |
| `"NormalizedPercentage"` | Parses percentage text back to 0.0-1.0 range. |
| `"Decibel"` | Parses dB text back to decibel value, with -100.0 for -INF. |
| `"Semitones"` | Parses semitone text back to a numeric semitone value. |

**Pitfalls:**
- An invalid mode string silently falls back to plain `getDoubleValue()` parsing, which extracts any leading numeric characters. No error or warning is produced for unrecognized modes.

**Cross References:**
- `$API.Engine.getTextForValue$`

## getVersion

**Signature:** `String getVersion()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a `String` from HISE settings (backend) or `FrontendHandler` static method (frontend), involving heap allocation for the String object.
**Minimal Example:** `var v = Engine.getVersion();`

**Description:**
Returns the product version string as configured in the project settings. In the HISE backend, reads from `HiseSettings::Project::Version` via the `GlobalSettingManager` settings object. In compiled plugins (frontend), reads from `FrontendHandler::getVersionString()`, which returns the version baked at export time. This is the plugin's own version (e.g., "1.0.0"), not the HISE engine version -- use `Engine.getProjectInfo().HISEBuild` for the HISE build version.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.getName$`
- `$API.Engine.getProjectInfo$`

## getWavetableList

**Signature:** `Array getWavetableList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new `Array<var>`, iterates wavetable list with String copies, and traverses the processor tree to find the first `WavetableSynth`.
**Minimal Example:** `var tables = Engine.getWavetableList();`

**Description:**
Returns a string array of all available wavetable names from the current expansion (or factory content). Finds the first `WavetableSynth` in the processor chain and retrieves its wavetable list. If no `WavetableSynth` exists in the signal chain, a script error is thrown.

**Parameters:**
None.

**Pitfalls:**
- Requires at least one `WavetableSynth` module in the signal chain. If none exists, a runtime script error is thrown ("You need at least one Wavetable synthesiser in your signal chain for this method"). This is not a silent failure -- the error message is descriptive.
- Always queries the first `WavetableSynth` found via `ProcessorHelpers::getFirstProcessorWithType`. If the project has multiple WavetableSynths with different wavetable sets, only the first one's list is returned.

**Cross References:**
- `$API.Engine.setCurrentExpansion$`
- `$API.Engine.getExpansionList$`

## getZoomLevel

**Disabled:** deprecated
**Disabled Reason:** Superseded by `Settings.getZoomLevel()`. The implementation calls `logSettingWarning("getZoomLevel")` which emits "Engine.getZoomLevel() is deprecated. Use Settings.getZoomLevel() instead." to the console, then returns the value.

## intToHexString

**Signature:** `String intToHexString(int value)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String construction via `String::toHexString()`, atomic ref-count operations on the returned String.
**Minimal Example:** `var hex = Engine.intToHexString(255);`

**Description:**
Converts an integer to its hexadecimal string representation using JUCE's `String::toHexString()`. Returns lowercase hex digits without a `0x` prefix (e.g., `255` returns `"ff"`, not `"0xff"`). Useful for debugging colour values, MIDI data, or bitmask operations.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Integer | no | The integer value to convert. | Any integer value. |

**Pitfalls:**
- The returned string uses lowercase hex digits with no `0x` prefix. To display as a colour constant (e.g., `0xFFFF0000`), you need to prepend `"0x"` manually.

**Cross References:**
- `$API.Engine.doubleToString$`

## isControllerUsedByAutomation

**Signature:** `int isControllerUsedByAutomation(var controllerNumber)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var idx = Engine.isControllerUsedByAutomation(1);`

**Description:**
Checks if a given MIDI CC number is assigned to a parameter automation and returns the index of the automation control, or -1 if not found. Accepts either a single integer (CC number on any channel) or a two-element array `[channel, ccNumber]` to specify a specific MIDI channel. When a single integer is passed, the channel is set to -1 (wildcard/any channel).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controllerNumber | Integer or Array | no | A single CC number (0-127) or a `[channel, ccNumber]` array. | When using an array, the first element is the MIDI channel (1-16), the second is the CC number (0-127). |

**Cross References:**
- `$API.Engine.createMidiAutomationHandler$`

## isHISE

**Signature:** `bool isHISE()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var inHise = Engine.isHISE();`

**Description:**
Returns `true` if the project is running inside the HISE IDE (backend), `false` in compiled plugins. This is a compile-time constant: the implementation returns `true` when `USE_BACKEND` is defined, `false` otherwise. Useful during development to conditionally enable debug features, console logging, or test code that should not run in the exported product.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.isPlugin$`
- `$API.Engine.getExtraDefinitionsInBackend$`

## isMpeEnabled

**Signature:** `bool isMpeEnabled()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var mpe = Engine.isMpeEnabled();`

**Description:**
Checks whether the global MPE (MIDI Polyphonic Expression) mode is currently enabled. Reads the state from the `MidiControlAutomationHandler`'s `MPEData` object via `getMPEData().isMpeEnabled()`. Returns `true` if MPE is active, `false` otherwise.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.createMidiAutomationHandler$`

## isPlugin

**Signature:** `bool isPlugin()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var plugin = Engine.isPlugin();`

**Description:**
Returns `true` if the application is running as a VST, AU, or AAX plugin, `false` if running as a standalone application. This is a compile-time constant: the implementation returns `false` when `IS_STANDALONE_APP` is defined, `true` otherwise. Useful for adjusting behavior based on the deployment context -- for example, enabling `Engine.quit()` only in standalone mode, or adjusting UI layout for plugin vs standalone windows.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.isHISE$`
- `$API.Engine.quit$`

## isUserPresetReadOnly

**Signature:** `bool isUserPresetReadOnly(var optionalFile)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File system operations in the frontend path (resolving preset paths, checking read-only status). String construction and I/O in the backend path (reading project settings).
**Minimal Example:** `var ro = Engine.isUserPresetReadOnly();`

**Description:**
Checks whether a user preset is read-only. Behavior differs by build target:

- **Backend:** Always returns the value of the `ReadOnlyFactoryPresets` project setting, ignoring the `optionalFile` parameter entirely. This reflects the intended production behavior where all factory presets shipped with the plugin are read-only.
- **Frontend (with `READ_ONLY_FACTORY_PRESETS` defined):** Checks a specific preset file. If `optionalFile` is omitted (undefined), checks the currently loaded preset. Accepts a `ScriptFile` object or a relative path string (resolved against the UserPresets directory, `.preset` extension added automatically). Delegates to `UserPresetHandler::isReadOnly()`.
- **Frontend (without `READ_ONLY_FACTORY_PRESETS`):** Always returns `false`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionalFile | String | no | A relative preset path, a ScriptFile object, or omit to check the currently loaded preset. The `.preset` extension is appended automatically for string paths. | Only effective in compiled plugins with `READ_ONLY_FACTORY_PRESETS` defined. |

**Pitfalls:**
- In the HISE backend, the `optionalFile` parameter is completely ignored -- the method always returns the global project setting regardless of which file is passed. This is by design (all backend presets are treated as factory presets) but may be confusing during development when testing per-file read-only behavior.

**Cross References:**
- `$API.Engine.loadUserPreset$`
- `$API.Engine.saveUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`

## loadAudioFileIntoBufferArray

**Signature:** `var loadAudioFileIntoBufferArray(String audioFileReference)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Performs file I/O (pool load from disk or cache), heap allocations (VariantBuffer creation, Array construction), and pool cache operations.
**Minimal Example:** `var channels = Engine.loadAudioFileIntoBufferArray("{PROJECT_FOLDER}myFile.wav");`

**Description:**
Loads an audio file and returns its content as an array of Buffer objects, one per channel. The audio file is resolved via a `PoolReference` with the `AudioFiles` file type, meaning it supports `{PROJECT_FOLDER}` and expansion wildcard references. The file is loaded through the audio sample buffer pool using `LoadAndCacheStrong`, so subsequent calls with the same reference return the cached data. If the current reference matches an expansion wildcard, the expansion's pool is used instead of the project pool.

Each element in the returned array is a `VariantBuffer` (the scripting `Buffer` type) wrapping the channel data. A stereo file returns a 2-element array; a mono file returns a 1-element array.

Reports a script error if the file cannot be loaded.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| audioFileReference | String | no | Pool reference string for the audio file. Supports `{PROJECT_FOLDER}` prefix and expansion wildcards. | Must resolve to a valid audio file in the pool. |

**Pitfalls:**
- The returned Buffer objects wrap the pool's internal memory directly (via `getWritePointer`). Modifying the buffer data modifies the cached pool entry, affecting all subsequent accesses to the same audio file reference.

**Cross References:**
- `$API.Engine.loadAudioFilesIntoPool$`

**Example:**

```javascript:load-audio-channels
// Title: Load audio file and read channel data
var channels = Engine.loadAudioFileIntoBufferArray("{PROJECT_FOLDER}impulse.wav");
Console.print("Channels: " + channels.length);
Console.print("Samples per channel: " + channels[0].length);
```

```json:testMetadata:load-audio-channels
{
  "testable": false,
  "skipReason": "Requires an audio file in the project's AudioFiles folder."
}
```

## loadAudioFilesIntoPool

**Signature:** `var loadAudioFilesIntoPool()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** File I/O (loads all audio files from disk in backend), heap allocations (pool loading, Array/String construction), extends compilation timeout.
**Minimal Example:** `var refs = Engine.loadAudioFilesIntoPool();`

**Description:**
Ensures all audio files are loaded into the audio sample buffer pool and returns a list of all pool reference strings. This serves two purposes:

1. **Backend:** If not all files are already loaded, loads every audio file from the project's AudioFiles folder into the pool with `LoadAndCacheStrong`. Extends the compilation timeout to prevent cancellation during long loads. Then falls through to the reference-listing step.
2. **Both targets:** Retrieves the list of all references from the audio sample buffer pool. If `FullInstrumentExpansion` is enabled and a current expansion is set, the expansion's pool is used instead of the project pool.

The returned array contains reference strings (e.g., `"{PROJECT_FOLDER}myFile.wav"`). This is important for compiled plugins: calling this method during `onInit` ensures all audio files referenced in the project are embedded and available at runtime.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.loadAudioFileIntoBufferArray$`

## loadFont

**Disabled:** deprecated
**Disabled Reason:** Use `Engine.loadFontAs()` instead. `loadFont` emits a `debugError` warning and delegates to `loadFontAs(fileName, "")`. The OS-dependent font name resolution makes `loadFont` unreliable across platforms -- `loadFontAs` accepts an explicit `fontId` that is consistent everywhere.

## loadFontAs

**Signature:** `void loadFontAs(String fileName, String fontId)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Performs file I/O (reads font file from disk in backend). In frontend builds this is a no-op since fonts are baked at compile time. Should be called during `onInit` to register fonts before they are used by UI components.
**Minimal Example:** `Engine.loadFontAs("MyFont.ttf", "CustomFont");`

**Description:**
Loads a font file from the project's Images folder and registers it under the given `fontId`. The `fontId` is the name you use to reference the font in component properties and `setGlobalFont()`. This is the platform-agnostic replacement for the deprecated `loadFont()`.

- **Backend:** Resolves the file name against the Images subdirectory via `GET_PROJECT_HANDLER`. Reads the entire file into a `MemoryBlock` and calls `MainController::loadTypeFace()` to register the font data under the given `fontId`. If `FullInstrumentExpansion` is enabled and a current expansion exists, the method returns early (fonts are loaded by the expansion system).
- **Frontend:** No-op. Fonts are embedded into the binary at export time and loaded automatically at startup. The call is harmless but unnecessary.

Reports a script error if the font file is not found in the backend.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | The font file name (e.g., "MyFont.ttf") relative to the Images folder. | Must exist in the project's Images directory (backend only). |
| fontId | String | no | The identifier to register the font under. Use this name when referencing the font in properties or `setGlobalFont()`. An empty string uses the font's internal name. | Should be unique and consistent across platforms. |

**Pitfalls:**
- The font file must be placed in the Images folder, not a Fonts or other custom folder. This is a common source of "File not found" errors.

**Cross References:**
- `$API.Engine.loadFont$`
- `$API.Engine.setGlobalFont$`

## loadFromJSON

**Signature:** `var loadFromJSON(String fileName)`
**Return Type:** `Object`
**Call Scope:** unsafe
**Call Scope Note:** File I/O (reads file from disk), JSON parsing (heap allocations, string construction).
**Minimal Example:** `var data = Engine.loadFromJSON("settings.json");`

**Description:**
Reads a JSON file from disk and returns the parsed object. If `fileName` is an absolute path, it is used directly. If it is a relative path, it is resolved against the UserPresets subdirectory of the project. This is the counterpart to `Engine.dumpAsJSON()`.

Returns an empty `var` (undefined) if the file does not exist. No error is reported for missing files -- the method silently returns undefined.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Absolute path or relative path (resolved against the UserPresets folder). | Should point to a valid JSON file. |

**Pitfalls:**
- If the file does not exist, the method silently returns `undefined` with no error message or console warning. Always check the return value with `isDefined()` before accessing properties.
- Relative paths are resolved against the UserPresets directory specifically, not the project root or Scripts folder. This matches `dumpAsJSON` behavior but may be unexpected.

**Cross References:**
- `$API.Engine.dumpAsJSON$`
- `$API.Engine.compressJSON$`
- `$API.Engine.uncompressJSON$`

## loadImageIntoPool

**Signature:** `void loadImageIntoPool(String id)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** File I/O (reads image files from disk), heap allocations (pool loading, image decoding), extends compilation timeout. Backend only -- no-op in compiled plugins.
**Minimal Example:** `Engine.loadImageIntoPool("{PROJECT_FOLDER}background.png");`

**Description:**
Loads one or more image files into the image pool. Backend only -- in compiled plugins this is a no-op since images are embedded at export time.

Supports wildcard patterns using `*`. When the `id` parameter contains `*`, the method scans the project's Images folder recursively and loads all files whose relative path contains the wildcard-stripped suffix. For example, `"{PROJECT_FOLDER}icons*"` loads all files in the Images folder whose path contains "icons". When no wildcard is present, the `id` is treated as a single pool reference and loaded directly.

If the reference matches an expansion wildcard, the expansion's image pool is used instead of the project pool. All loaded images use `LoadAndCacheStrong` for persistent caching. Extends the compilation timeout to prevent cancellation during bulk loads.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | A pool reference string (e.g., `"{PROJECT_FOLDER}image.png"`) or a wildcard pattern (e.g., `"{PROJECT_FOLDER}icons*"`). | Backend only. |

**Pitfalls:**
- This method does nothing in compiled plugins. To ensure images are available in the exported plugin, they must be referenced by UI components or explicitly included in the project.
- The wildcard matching is a simple `contains()` check on the relative path, not a true glob pattern. `"*icons"` and `"icons*"` will both match any file whose path contains "icons".

**Cross References:**
- `$API.Engine.loadAudioFilesIntoPool$`

## loadNextUserPreset

**Signature:** `void loadNextUserPreset(bool stayInDirectory)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `UserPresetHandler::incPreset()` which involves file system access, preset loading, ValueTree operations, and string construction.
**Minimal Example:** `Engine.loadNextUserPreset(true);`

**Description:**
Loads the next user preset in the preset list. Delegates to `UserPresetHandler::incPreset(true, stayInDirectory)`. The `stayInDirectory` parameter controls whether navigation wraps within the current preset subfolder or traverses the entire flat preset list. When `true`, only presets in the same directory as the currently loaded preset are considered; when `false`, the full user preset list is navigated.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| stayInDirectory | Integer | no | If `true`, navigation stays within the current preset subfolder. If `false`, navigates across the entire preset list. | -- |

**Cross References:**
- `$API.Engine.loadPreviousUserPreset$`
- `$API.Engine.loadUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`
- `$API.Engine.getUserPresetList$`

## loadPreviousUserPreset

**Signature:** `void loadPreviousUserPreset(bool stayInDirectory)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `UserPresetHandler::incPreset()` which involves file system access, preset loading, ValueTree operations, and string construction.
**Minimal Example:** `Engine.loadPreviousUserPreset(true);`

**Description:**
Loads the previous user preset in the preset list. Delegates to `UserPresetHandler::incPreset(false, stayInDirectory)`. The `stayInDirectory` parameter controls whether navigation wraps within the current preset subfolder or traverses the entire flat preset list. When `true`, only presets in the same directory as the currently loaded preset are considered; when `false`, the full user preset list is navigated.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| stayInDirectory | Integer | no | If `true`, navigation stays within the current preset subfolder. If `false`, navigates across the entire preset list. | -- |

**Cross References:**
- `$API.Engine.loadNextUserPreset$`
- `$API.Engine.loadUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`
- `$API.Engine.getUserPresetList$`

## loadUserPreset

**Signature:** `void loadUserPreset(var relativePathOrFileObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Involves file system access, ValueTree parsing, preset restoration, and string construction. Explicitly rejects calls during initialization.
**Minimal Example:** `Engine.loadUserPreset("Pads/Warm Pad");`

**Description:**
Loads a user preset by relative path string or `ScriptFile` object. Relative paths are resolved against the UserPresets directory (backend: project's UserPresets subfolder; frontend: the platform-specific user preset directory). The `.preset` extension is appended automatically if not present. Absolute paths are also accepted. This method explicitly checks `MainController::isInitialised()` and throws a script error if called during `onInit` -- user presets must be loaded from runtime callbacks (button handlers, timer callbacks, etc.), not during initialization.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| relativePathOrFileObject | String | no | Relative path from the UserPresets directory (use `/` as directory separator, no `.preset` extension needed), or a `ScriptFile` object pointing to the preset file. | Must resolve to an existing `.preset` file. |

**Pitfalls:**
- Calling this method during `onInit` throws a script error ("Do not load user presets at startup."). Load presets from runtime callbacks such as button handlers or timer callbacks instead.

**Cross References:**
- `$API.Engine.saveUserPreset$`
- `$API.Engine.loadNextUserPreset$`
- `$API.Engine.loadPreviousUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`
- `$API.Engine.getUserPresetList$`
- `$API.Engine.createUserPresetHandler$`

## logSettingWarning

**Signature:** `void logSettingWarning(String methodName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a `String` (heap allocation) and writes to the console via `debugToConsole`.
**Minimal Example:** `Engine.logSettingWarning("getZoomLevel");`

**Description:**
Internal deprecation helper that emits a console message of the form `"Engine.{methodName}() is deprecated. Use Settings.{methodName}() instead."`. This method is called internally by deprecated Engine methods (`getZoomLevel`, `setZoomLevel`, `setDiskMode`) to warn users to migrate to the Settings class. While technically callable from user scripts, it serves no practical purpose -- it only prints a migration warning to the console.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| methodName | String | no | The name of the deprecated Engine method to include in the warning message. | -- |

**Cross References:**
- `$API.Engine.getZoomLevel$`
- `$API.Engine.setZoomLevel$`
- `$API.Engine.setDiskMode$`

## matchesRegex

**Signature:** `bool matchesRegex(String stringToMatch, String regex)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Constructs `std::string` and `std::regex` objects (heap allocation). Regex compilation is an expensive operation.
**Minimal Example:** `var ok = Engine.matchesRegex("hello123", "[a-z]+\\d+");`

**Description:**
Tests whether the given string matches the provided regular expression pattern. Uses `std::regex_search` internally, so a partial match anywhere in the string is sufficient (the regex does not need to match the entire string). If the regex pattern is invalid, a debug error is emitted to the console and `false` is returned. The regex engine is ECMAScript (the C++ `std::regex` default).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| stringToMatch | String | no | The string to test against the regex pattern. | -- |
| regex | String | no | The regular expression pattern (ECMAScript syntax). | Must be a valid regex; invalid patterns produce a console error. |

**Pitfalls:**
- This method uses `std::regex_search`, not `std::regex_match`. The pattern does not need to cover the full string -- any substring match returns `true`. To require a full-string match, anchor the pattern with `^` and `$`.

**Cross References:**
- `$API.Engine.getRegexMatches$`

## openWebsite

**Signature:** `void openWebsite(String url)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a JUCE `URL` object (heap allocation, string parsing), then schedules a `DelayedFunctionCaller` with a 300ms delay that launches the system browser.
**Minimal Example:** `Engine.openWebsite("https://hise.dev");`

**Description:**
Opens the given URL in the system's default web browser. The URL is validated using JUCE's `URL::isWellFormed()` check -- if the URL is malformed, a script error is thrown. The browser launch is deferred by 300ms using a `DelayedFunctionCaller` to avoid blocking the script execution context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| url | String | no | The URL to open in the system browser. | Must be a well-formed URL (validated by `URL::isWellFormed()`). |

**Cross References:**
None.

## performUndoAction

**Signature:** `bool performUndoAction(JSON thisObject, Function undoAction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Creates a `ScriptUndoableAction` on the heap (via `new`), registers it with the `UndoManager`, and the callback dispatch involves `WeakCallbackHolder` with `callSync` or `call` depending on the thread.
**Minimal Example:** `Engine.performUndoAction({}, onUndoAction);`

**Description:**
Registers and immediately performs a scriptable undo action. The `undoAction` callback is wrapped in a `ScriptUndoableAction` (a JUCE `UndoableAction` subclass) and submitted to the control undo manager. The callback receives a single boolean argument: `false` when the action is first performed, and `true` when it is undone via `Engine.undo()`. The `thisObject` parameter is bound as the `this` context for the callback. Thread-aware dispatch: on the scripting or sample-loading thread the callback executes synchronously; on the message thread it executes asynchronously. The callback is marked as high-priority to prefer synchronous execution when possible.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| thisObject | JSON | yes | The object to use as `this` context inside the callback. Pass `{}` or a state object. | -- |
| undoAction | Function | yes | Callback function invoked on perform and undo. | Must accept 1 argument (isUndo: bool). |

**Callback Signature:** undoAction(isUndo: bool)

**Example:**
```javascript:perform-undo-action
// Title: Undo/redo a slider value change
const var knob = Content.addKnob("UndoKnob", 0, 0);
knob.set("saveInPreset", false);

var previousValue = 0.5;

inline function onUndoAction(isUndo)
{
    if (isUndo)
        knob.setValue(previousValue);
    else
        knob.setValue(1.0);
};

Engine.performUndoAction({}, onUndoAction);
```
```json:testMetadata:perform-undo-action
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "knob.getValue()", "value": 1.0}
  ]
}
```

**Pitfalls:**
- On the message thread, the callback is dispatched asynchronously, meaning the action may not be complete when `performUndoAction` returns. On the scripting thread and sample-loading thread it executes synchronously.

**Cross References:**
- `$API.Engine.undo$`
- `$API.Engine.redo$`
- `$API.Engine.clearUndoHistory$`

## playBuffer

**Signature:** `void playBuffer(var bufferData, var callback, double fileSampleRate)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Lazily creates a `PreviewHandler` on first call (heap allocation, listener registration). Each invocation creates a `Job` object (heap allocation), stops any currently playing preview, and starts a `PooledUIUpdater::SimpleTimer` for progress callbacks.
**Minimal Example:** `Engine.playBuffer(myBuffer, onPlaybackState, 44100.0);`

**Description:**
Previews an audio buffer through the engine's main output with a progress callback. The `bufferData` parameter accepts either a single `Buffer` object (mono -- automatically duplicated to stereo) or an `Array` of `Buffer` objects (one per channel). The `callback` function is called periodically during playback with two arguments: a boolean indicating whether playback is active, and a normalized position (0.0 to 1.0). When playback ends (either naturally or by calling `playBuffer` again with new data), the callback fires one final time with `(false, 1.0)`. If `fileSampleRate` is 0 or negative, the current engine sample rate is used. The `PreviewHandler` is created lazily on first call and persists for the lifetime of the Engine object. Calling `playBuffer` while a preview is already playing stops the current preview and starts the new one.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| bufferData | Buffer | no | A single `Buffer` (mono, auto-duplicated to stereo) or an `Array` of `Buffer` objects (one per channel). | Must contain valid buffer data with non-zero length. |
| callback | Function | no | Progress callback called periodically during playback. | Must accept 2 arguments (isPlaying, position). |
| fileSampleRate | Double | no | The sample rate of the buffer data. If <= 0, the engine's current sample rate is used. | -- |

**Callback Signature:** callback(isPlaying: bool, position: double)

**Example:**
```javascript:play-buffer-preview
// Title: Preview an audio buffer with progress tracking
const var buffer = Engine.loadAudioFileIntoBufferArray("{PROJECT_FOLDER}audiofile.wav");
var playbackProgress = 0.0;

inline function onPlaybackState(isPlaying, position)
{
    playbackProgress = position;
    if (!isPlaying)
        Console.print("Playback finished");
};

Engine.playBuffer(buffer, onPlaybackState, 44100.0);
```
```json:testMetadata:play-buffer-preview
{
  "testable": false,
  "skipReason": "Requires an audio file in the project folder and active audio output."
}
```

**Pitfalls:**
- Mono buffers are automatically duplicated to stereo (channel 0 is copied to channel 1). There is no way to play a true mono preview -- it always outputs at least 2 channels.

**Cross References:**
- `$API.Engine.loadAudioFileIntoBufferArray$`
- `$API.Engine.renderAudio$`

## quit

**Signature:** `void quit()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `JUCEApplication::quit()` which triggers the application shutdown sequence (message thread operations, resource cleanup).
**Minimal Example:** `Engine.quit();`

**Description:**
Signals that the standalone application should terminate. This calls `JUCEApplication::quit()` which posts a quit message to the JUCE message loop, triggering a graceful shutdown. The method is completely compiled out (empty body) when `IS_STANDALONE_APP` is not defined -- in VST/AU/AAX plugin builds, calling this method does nothing.

**Parameters:**
None.

**Pitfalls:**
- This method is a complete no-op in plugin builds (VST/AU/AAX). It only works in standalone applications. No error or warning is produced when called in a plugin context.

**Cross References:**
- `$API.Engine.isPlugin$`

## rebuildCachedPools

**Signature:** `void rebuildCachedPools()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `clearData()` and `loadAllFilesFromProjectFolder()` on the MIDI file and sample map pools, which involve file I/O, heap allocations, and string construction.
**Minimal Example:** `Engine.rebuildCachedPools();`

**Description:**
Clears and reloads the MIDI file pool and sample map pool from the project folder. This is a backend-only (HISE IDE) operation -- the entire method body is compiled out in exported plugins. Useful when MIDI files or sample maps have been added, removed, or modified externally while HISE is running.

**Parameters:**
None.

**Pitfalls:**
- This method is a complete no-op in compiled plugins (VST/AU/AAX/standalone). The entire implementation is inside a `#if USE_BACKEND` guard. No error or warning is produced when called in a frontend build.

**Cross References:**
- `$API.Engine.clearMidiFilePool$`
- `$API.Engine.clearSampleMapPool$`

## redo

**Signature:** `void redo()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** May dispatch to the message thread via `MessageManager::callAsync`. The synchronous path (script transactions) calls `UndoManager::redo()` which involves heap allocations for the undo action execution.
**Minimal Example:** `Engine.redo();`

**Description:**
Redoes the last undone action from the undo manager. The method has two execution paths based on the current redo description: if the next redo action is a script transaction (marked with `%SCRIPT_TRANSACTION%`), it executes synchronously on the current thread. Otherwise, it dispatches the redo operation to the message thread via `MessageManager::callAsync`. This is the counterpart to `Engine.undo()`.

**Parameters:**
None.

**Cross References:**
- `$API.Engine.undo$`
- `$API.Engine.performUndoAction$`
- `$API.Engine.clearUndoHistory$`

## reloadAllSamples

**Signature:** `void reloadAllSamples()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Performs file I/O to verify sample directories, kills all voices, then dispatches sample map reloading to the sample loading thread via `killVoicesAndCall`.
**Minimal Example:** `Engine.reloadAllSamples();`

**Description:**
Forces a full asynchronous reload of all samples across every `ModulatorSampler` in the processor tree. The method first checks sample subdirectories (in backend, calls `checkSubDirectories()`; in frontend, calls `checkAllSampleReferences()`), then uses `killVoicesAndCall()` to safely kill all voices and dispatch the reload to the sample loading thread. Each `ModulatorSampler` found via a `Processor::Iterator` has its `reloadSampleMap()` called. This is useful after the sample directory has changed or sample files have been modified externally.

**Parameters:**
None.

**Pitfalls:**
- This method kills all active voices before reloading. Any notes currently playing will be cut off immediately with no release phase.

**Cross References:**
- `$API.Engine.setAllowDuplicateSamples$`

## renderAudio

**Signature:** `void renderAudio(var eventList, var finishCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new `AudioRenderer` thread (extends `Thread`) and stores it in a member `ScopedPointer`, involving heap allocation and thread spawning.
**Minimal Example:** `Engine.renderAudio(eventList, onRenderUpdate);`

**Description:**
Renders a list of MIDI events to audio buffers on a background thread. The `eventList` must be an array of `MessageHolder` objects (created via `Engine.createMessageHolder()`). The callback is called periodically during rendering with a status object and once more when rendering completes. The callback receives a single argument -- a JSON object with `channels` (array of Buffer objects, one per channel), `finished` (boolean indicating completion), and `progress` (0.0-1.0 double). Events are split into multiple internal buffers at `HISE_EVENT_BUFFER_SIZE` boundaries. The audio thread ID is temporarily removed during the callback to allow script execution from the background render thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventList | Array | no | Array of MessageHolder objects containing the MIDI events to render. | Must be an Array of MessageHolder objects. |
| finishCallback | Function | no | Callback invoked during and after rendering with a status object. | -- |

**Callback Signature:** finishCallback(status: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| channels | Array | Array of Buffer objects (one per output channel) containing the rendered audio data. |
| finished | bool | `true` when rendering is complete, `false` during progress updates. |
| progress | double | Rendering progress from 0.0 to 1.0. |

**Pitfalls:**
- Calling `renderAudio` again while a render is in progress replaces the current render thread (the `ScopedPointer` assignment destroys the previous `AudioRenderer`). The previous render's callback will not receive a final `finished: true` call.

**Cross References:**
- `$API.Engine.createMessageHolder$`
- `$API.Engine.playBuffer$`

**Example:**
```javascript:render-midi-to-audio
// Title: Render MIDI note to audio buffer
const var noteOn = Engine.createMessageHolder();
noteOn.setType(noteOn.cycleId);
noteOn.setNoteNumber(60);
noteOn.setVelocity(127);
noteOn.setChannel(1);

const var noteOff = Engine.createMessageHolder();
noteOff.setType(noteOff.cycleId);
noteOff.setNoteNumber(60);
noteOff.setVelocity(0);
noteOff.setChannel(1);
noteOff.setTimestamp(44100);

var eventList = [noteOn, noteOff];

inline function onRenderDone(status)
{
    if (status.finished)
    {
        Console.print("Render complete: " + status.channels.length + " channels");
    }
}

Engine.renderAudio(eventList, onRenderDone);
```
```json:testMetadata:render-midi-to-audio
{
  "testable": false,
  "skipReason": "Requires audio processing engine to render MIDI events to audio buffers; background thread timing is non-deterministic."
}
```

## saveUserPreset

**Signature:** `void saveUserPreset(var presetName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Performs file I/O to save the preset to disk. Involves string construction, ValueTree serialization, and file system operations.
**Minimal Example:** `Engine.saveUserPreset("MyPreset");`

**Description:**
Saves the current plugin state as a user preset. Accepts either a string name or a `ScriptFile` object. When a `ScriptFile` is passed, the preset is saved directly to that file path using `UserPresetHelpers::saveUserPreset`. When a string is passed (or an empty string), the method delegates to `UserPresetHandler::savePreset()` -- if the string is empty, the user is prompted to enter a name.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| presetName | String | no | The preset name to save as, or a ScriptFile object for the target path. An empty string triggers a name prompt. | -- |

**Cross References:**
- `$API.Engine.loadUserPreset$`
- `$API.Engine.getCurrentUserPresetName$`
- `$API.Engine.getUserPresetList$`
- `$API.Engine.addModuleStateToUserPreset$`
- `$API.Engine.createUserPresetHandler$`

## setAllowDuplicateSamples

**Signature:** `void setAllowDuplicateSamples(bool shouldAllow)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls through to `ModulatorSamplerSoundPool::setAllowDuplicateSamples`, which modifies a flag accessed during sample loading (a thread-unsafe context). Should be called during `onInit`.
**Minimal Example:** `Engine.setAllowDuplicateSamples(false);`

**Description:**
Controls whether the sample pool allows duplicate sample references. When set to `false`, the pool deduplicates samples -- if two samplers reference the same audio file, only one copy is loaded into memory and both samplers share it. When set to `true`, each sampler loads its own independent copy, allowing different processing of the same source file (e.g., different start offsets or gain applied at the buffer level). The default is typically `true` (duplicates allowed).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldAllow | Integer | no | `true` to allow duplicate sample references (independent copies), `false` to deduplicate and share buffers. | -- |

**Cross References:**
- `$API.Engine.reloadAllSamples$`

## setCurrentExpansion

**Signature:** `bool setCurrentExpansion(String expansionName)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `ExpansionHandler::setCurrentExpansion`, which involves string comparisons, expansion list iteration, and notification dispatch (ValueTree property changes, listener callbacks).
**Minimal Example:** `var ok = Engine.setCurrentExpansion("MyExpansion");`

**Description:**
Sets the active expansion by name and updates the preset browser to reflect the change. Delegates directly to `ExpansionHandler::setCurrentExpansion()`. Returns `true` if the expansion was found and activated, `false` otherwise. Passing an empty string deactivates the current expansion.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| expansionName | String | no | The name of the expansion to activate, or an empty string to deactivate. | Must match an installed expansion name. |

**Cross References:**
- `$API.Engine.getExpansionList$`
- `$API.Engine.createExpansionHandler$`

## setDiskMode

**Disabled:** deprecated
**Disabled Reason:** Use `Settings.setDiskMode()` instead. The method emits a deprecation warning via `logSettingWarning()` then still executes -- it is soft-deprecated, not hard-deprecated.

## setFrontendMacros

**Signature:** `void setFrontendMacros(var nameList)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies macro manager state (`setEnableMacroOnFrontend`, `setMacroName`) which involves string construction and internal data structure updates.
**Minimal Example:** `Engine.setFrontendMacros(["Macro 1", "Macro 2", "Macro 3", "Macro 4"]);`

**Description:**
Enables the macro system for the end user and assigns names to each macro slot. The `nameList` must be an array of strings. Passing a non-empty array enables macros on the frontend; passing an empty array disables them. The method iterates up to `HISE_NUM_MACROS` slots (default 8, configurable via preprocessor) and assigns each name from the array. If the array has fewer entries than `HISE_NUM_MACROS`, the remaining slots receive empty-string names. If the array is not an Array type, macros are disabled and a script error is reported.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| nameList | Array | no | Array of strings with names for each macro slot. | Must be an Array. Length should match `HISE_NUM_MACROS` (default 8). |

**Pitfalls:**
- If the array contains fewer entries than `HISE_NUM_MACROS`, the excess slots silently receive empty-string names with no warning. The method does not validate the array length against the macro count.

**Cross References:**
- `$API.Engine.getMacroName$`
- `$API.Engine.createMacroHandler$`

## setGlobalFont

**Signature:** `void setGlobalFont(String fontName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `MainController::setGlobalFont()` which constructs Font objects from string lookup (`getFontFromString`), assigns a font to the global LookAndFeel. String construction and font resolution are heap operations.
**Minimal Example:** `Engine.setGlobalFont("Oxygen Bold");`

**Description:**
Sets the default font used throughout the plugin for UI elements such as labels, combo boxes, and other components that use the global font. The font must have been previously loaded via `Engine.loadFontAs()` (or be a system font). If `fontName` is an empty string, the global font resets to the default HISE font (`GLOBAL_FONT()`). The method also updates the combo box font in the main LookAndFeel.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fontName | String | no | The name of a previously loaded font, or an empty string to reset to default. | Must match a loaded font ID or be empty. |

**Pitfalls:**
- If the font name does not match any loaded font, the method silently falls back to a system font lookup with no warning. The UI will use whatever font the OS resolves, which may differ across platforms.

**Cross References:**
- `$API.Engine.loadFontAs$`
- `$API.Engine.loadFont$`

## setGlobalPitchFactor

**Signature:** `void setGlobalPitchFactor(double pitchFactorInSemitones)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Clamps the input with `jlimit`, then writes a single `double` member variable (`globalPitchFactor = pow(2, semitones/12.0)`). No allocations, no locks.
**Minimal Example:** `Engine.setGlobalPitchFactor(-2.0);`

**Description:**
Sets the global pitch factor in semitones, affecting the pitch of all voices in the instrument. The value is clamped to the range -12.0 to +12.0 semitones (one octave down to one octave up). Internally, the semitone value is converted to a pitch ratio via `pow(2, semitones / 12.0)` and stored as the global pitch factor that voice renderers multiply into their frequency calculations.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pitchFactorInSemitones | Double | no | The global pitch offset in semitones. | Clamped to -12.0 .. 12.0. |

**Cross References:**
- `$API.Engine.getGlobalPitchFactor$`

## setHostBpm

**Signature:** `void setHostBpm(Number newTempo)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes a single `double` member variable (`globalBPM`). No allocations, no locks, no downstream notifications in this method.
**Minimal Example:** `Engine.setHostBpm(140.0);`

**Description:**
Overwrites the host BPM with a fixed value. All tempo-based calculations (tempo sync, quarter beat conversions, etc.) will use this value instead of the host-reported tempo. Pass -1 to re-enable sync to the host DAW's tempo. The value is written directly to `GlobalSettingManager::globalBPM` with no validation or clamping.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newTempo | Number | yes | The BPM value to use, or -1 to sync to host. | -1 for host sync, or a positive BPM value. |

**Pitfalls:**
- Passing 0 or any non-negative value below a reasonable BPM will not produce an error. The value is stored as-is and will cause division-by-zero or extreme results in tempo-based conversions.

**Cross References:**
- `$API.Engine.getHostBpm$`
- `$API.Engine.createTransportHandler$`

## setKeyColour

**Signature:** `void setKeyColour(int keyNumber, int colourAsHex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `CustomKeyboardState::setColourForSingleKey()` which writes to an internal array and then calls `sendChangeMessage()` (a `ChangeBroadcaster` operation that may acquire a lock and dispatch to listeners).
**Minimal Example:** `Engine.setKeyColour(60, 0xFFFF0000);`

**Description:**
Sets the colour of a single key on the on-screen MIDI keyboard. The key number is a standard MIDI note number (0-127). The colour is specified as a 32-bit integer in `0xAARRGGBB` format. To clear a key's custom colour, pass `0x00000000` (transparent black). After setting the colour, a change message is broadcast to update the keyboard display.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyNumber | Integer | no | The MIDI note number of the key to colour. | 0-127. Values outside this range are silently ignored. |
| colourAsHex | Integer | no | The colour in `0xAARRGGBB` format. | Full 32-bit colour with alpha. |

**Cross References:**
- `$API.Engine.setLowestKeyToDisplay$`

## setLatencySamples

**Signature:** `void setLatencySamples(int latency)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `AudioProcessor::setLatencySamples()` which notifies the host of the latency change. The JUCE implementation may trigger host callbacks and message thread dispatching.
**Minimal Example:** `Engine.setLatencySamples(512);`

**Description:**
Sets the plugin's reported latency in samples. The host DAW uses this value to compensate for processing delays (plugin delay compensation). The default latency is 0. This wraps JUCE's `AudioProcessor::setLatencySamples()` directly, so the host is notified immediately of the change.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| latency | Integer | no | The latency in samples to report to the host. | Should be >= 0. |

**Cross References:**
- `$API.Engine.getLatencySamples$`

## setLowestKeyToDisplay

**Signature:** `void setLowestKeyToDisplay(int keyNumber)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes a single integer member variable (`lowestKey`). No allocations, no locks, no change message broadcast.
**Minimal Example:** `Engine.setLowestKeyToDisplay(36);`

**Description:**
Sets the lowest visible key on the on-screen MIDI keyboard, effectively scrolling the keyboard display to start at the given note number. The value is stored in `CustomKeyboardState::lowestKey` and the keyboard component reads it during rendering. Unlike `setKeyColour`, this method does not broadcast a change message -- the keyboard picks up the new value on its next repaint.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyNumber | Integer | no | The MIDI note number to use as the lowest visible key. | 0-127. No range validation is performed. |

**Cross References:**
- `$API.Engine.setKeyColour$`

## setMaximumBlockSize

**Signature:** `void setMaximumBlockSize(Number numSamplesPerBlock)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls `MainController::setMaximumBlockSize()` which may call `prepareToPlay()`, re-initializing the entire audio processing chain with new buffer allocations. This is a heavyweight operation that should only be done during initialization.
**Minimal Example:** `Engine.setMaximumBlockSize(256);`

**Description:**
Sets the maximum buffer size that HISE uses for its internal processing. If the host's audio buffer is larger than this value, HISE splits the incoming buffer into multiple chunks of this size and processes each chunk separately. The value is rounded down to the nearest multiple of `HISE_EVENT_RASTER` (default 8) and clamped to the range 16 to `HISE_MAX_PROCESSING_BLOCKSIZE` (default 512). If the new value differs from the current maximum block size and the audio engine is already initialized, `prepareToPlay()` is called to re-initialize the entire processing chain. A smaller block size increases the control rate resolution (more frequent modulation updates) at the cost of higher CPU usage due to more frequent processing calls and reduced SIMD efficiency.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numSamplesPerBlock | Number | yes | The maximum number of samples per processing block. | Rounded down to nearest multiple of `HISE_EVENT_RASTER` (8). Clamped to 16 .. `HISE_MAX_PROCESSING_BLOCKSIZE` (512). |

**Pitfalls:**
- The value is silently rounded down to the nearest multiple of `HISE_EVENT_RASTER` (default 8). Passing a value like 100 results in an effective block size of 96 with no warning.

**Cross References:**
- `$API.Engine.getBufferSize$`
- `$API.Engine.getControlRateDownsamplingFactor$`
- `$API.Engine.setMinimumSampleRate$`

## setPreloadMessage

**Signature:** `void setPreloadMessage(String message)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `SampleManager::setPreloadMessage()` which involves string construction and internal state changes on the sample loading system.
**Minimal Example:** `Engine.setPreloadMessage("Loading instruments...");`

**Description:**
Sets the preload message that is displayed during sample loading. This allows scripts to provide custom progress messages to users while samples are being preloaded or during other long-running operations that use the preload overlay. The message is retrievable via `Engine.getPreloadMessage()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| message | String | no | The message text to display during preloading. | -- |

**Cross References:**
- `$API.Engine.getPreloadMessage$`
- `$API.Engine.getPreloadProgress$`

## setMinimumSampleRate

**Signature:** `bool setMinimumSampleRate(Number minimumSampleRate)`
**Return Type:** `Integer`
**Call Scope:** init
**Call Scope Note:** Calls `MainController::setMinimumSamplerate()` which calls `refreshOversampling()`, re-initializing the entire oversampling chain. This is a heavyweight operation that should only be done during initialization.
**Minimal Example:** `var changed = Engine.setMinimumSampleRate(96000.0);`

**Description:**
Sets the minimum sample rate for the global audio processing chain. If the current audio device sample rate is lower than the specified minimum, HISE enables oversampling to bring the effective processing rate up to at least the specified minimum. The value is clamped to the range 1.0 to 384000.0 (96000 * 4). Returns `true` if the oversampling configuration changed as a result, `false` if no change was needed (i.e., the current sample rate already meets or exceeds the minimum, or the oversampling factor remains the same).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| minimumSampleRate | Number | yes | The minimum sample rate for internal processing. If the device sample rate is below this, oversampling is enabled. | Clamped to 1.0 .. 384000.0 |

**Cross References:**
- `$API.Engine.getSampleRate$`
- `$API.Engine.setMaximumBlockSize$`

## setUserPresetTagList

**Signature:** `void setUserPresetTagList(Array listOfTags)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies the tag database (`StringArray` construction, heap operations) in the `UserPresetHandler`.
**Minimal Example:** `Engine.setUserPresetTagList(["Bass", "Lead", "Pad", "FX"]);`

**Description:**
Sets the list of tags that appear in the user preset browser for filtering presets. Each element in the array is converted to a string via `toString()`. The tags are stored in the `UserPresetHandler`'s tag database. This is typically called during `onInit` to configure preset browser categories before the user interacts with the preset system.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| listOfTags | Array | no | An array of strings defining the available tag categories for the preset browser. | Must be an array; non-array values are silently ignored. |

**Pitfalls:**
- If the argument is not an array (e.g., a single string), the method silently does nothing. No error is reported.

**Cross References:**
- `$API.Engine.getUserPresetList$`
- `$API.Engine.loadUserPreset$`

## setZoomLevel

**Disabled:** deprecated
**Disabled Reason:** Superseded by `Settings.setZoomLevel()`. The implementation calls `logSettingWarning("setZoomLevel")` which emits a console deprecation message, then proceeds to set the zoom level via `GlobalSettingManager::setGlobalScaleFactor()`. The method still works but warns users to migrate to the Settings class. The value is clamped to 0.25 .. 2.0.

## showErrorMessage

**Signature:** `void showErrorMessage(String message, bool isCritical)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `MainController::sendOverlayMessage()` which involves string construction and UI overlay state management.
**Minimal Example:** `Engine.showErrorMessage("Sample folder not found", true);`

**Description:**
Shows an error message as an overlay on the compiled plugin interface. When `isCritical` is `true`, the overlay uses `CriticalCustomErrorMessage` state which disables the "Ignore" button, forcing the user to acknowledge the error before proceeding. When `isCritical` is `false`, the overlay uses `CustomErrorMessage` state which includes an "Ignore" button allowing the user to dismiss the message and continue. In the HISE IDE, the message is displayed in the console.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| message | String | no | The error message text to display. | -- |
| isCritical | Integer | no | If `true`, the Ignore button is disabled, preventing the user from dismissing the error. | -- |

**Cross References:**
- `$API.Engine.showMessage$`
- `$API.Engine.showMessageBox$`

## showMessage

**Signature:** `void showMessage(String message)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `MainController::sendOverlayMessage()` which involves string construction and UI overlay state management.
**Minimal Example:** `Engine.showMessage("Preset loaded successfully.");`

**Description:**
Shows an informational message as an overlay on the compiled plugin interface with an "OK" button. Uses the `CustomInformation` overlay state, which presents a non-error notification suitable for informing the user about important events (e.g., successful operations, status updates). The overlay blocks interaction with the plugin until the user clicks "OK".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| message | String | no | The informational message text to display. | -- |

**Cross References:**
- `$API.Engine.showErrorMessage$`
- `$API.Engine.showMessageBox$`
- `$API.Engine.showYesNoWindow$`

## showMessageBox

**Signature:** `void showMessageBox(String title, String markdownMessage, int type)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Dispatches to the message thread via `MessageManager::callAsync` which involves heap allocation for the lambda capture and cross-thread dispatch.
**Minimal Example:** `Engine.showMessageBox("Info", "Operation completed.", 0);`

**Description:**
Shows a modal message box with a title, a markdown-formatted message body, and an OK button. The icon displayed is determined by the `type` parameter, which maps to `PresetHandler::IconType`. The dialog is dispatched asynchronously to the message thread via `MessageManager::callAsync`, so the calling script does not block. The message body supports markdown formatting.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| title | String | no | The title text of the message box window. | -- |
| markdownMessage | String | no | The message body, which supports markdown formatting. | -- |
| type | Integer | no | The icon type to display. | 0 = Info, 1 = Warning, 2 = Question, 3 = Error |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| 0 | Info icon -- used for general informational messages. |
| 1 | Warning icon -- used for non-critical warnings. |
| 2 | Question icon -- used for queries (though this box has no response mechanism; use `showYesNoWindow` for interactive dialogs). |
| 3 | Error icon -- used for error notifications. |

**Cross References:**
- `$API.Engine.showMessage$`
- `$API.Engine.showErrorMessage$`
- `$API.Engine.showYesNoWindow$`

## showYesNoWindow

**Signature:** `void showYesNoWindow(String title, String markdownMessage, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Dispatches to the message thread via `MessageManager::callAsync` which involves heap allocation for the lambda capture, `WeakCallbackHolder` construction, and cross-thread dispatch.
**Minimal Example:** `Engine.showYesNoWindow("Confirm", "Are you sure?", ok => Console.print(ok));`

**Description:**
Shows a modal yes/no dialog with a title and a markdown-formatted message body, then executes the callback function with the user's choice. The dialog is dispatched asynchronously to the message thread via `MessageManager::callAsync`. After the user clicks Yes or No, the callback is invoked with a single boolean argument: `true` if the user clicked Yes, `false` if they clicked No. The callback is executed via a `WeakCallbackHolder` with 1 expected argument.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| title | String | no | The title text of the dialog window. | -- |
| markdownMessage | String | no | The message body, which supports markdown formatting. | -- |
| callback | Function | no | Function called after the user makes a choice. Receives a single boolean argument. | Must accept 1 argument. |

**Callback Signature:** callback(ok: bool)

**Cross References:**
- `$API.Engine.showMessage$`
- `$API.Engine.showErrorMessage$`
- `$API.Engine.showMessageBox$`

**Example:**
```javascript:yes-no-confirm-action
// Title: Confirm a destructive action with showYesNoWindow
Engine.showYesNoWindow("Delete Preset", "Are you sure you want to delete this preset?", function(ok)
{
    if (ok)
        Console.print("User confirmed deletion");
    else
        Console.print("User cancelled");
});
```
```json:testMetadata:yes-no-confirm-action
{
  "testable": false,
  "skipReason": "Requires user interaction with a modal dialog on the message thread."
}
```

## sortWithFunction

**Signature:** `bool sortWithFunction(var arrayToSort, var sortFunction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Calls `callExternalFunctionRaw` repeatedly during the sort, which executes script function calls. The sort itself operates on the array in-place and involves heap-level JUCE array manipulation.
**Minimal Example:** `Engine.sortWithFunction(myArray, onCompare);`

**Description:**
Sorts an array in-place using a custom comparison function. The comparator receives two elements and must return a negative number if the first should come before the second, zero if equal, or a positive number if the first should come after the second (standard C-style comparator convention). Returns `true` if the sort succeeded, or `false` if either argument is invalid (not an array or not a function). The array is modified in-place -- no copy is created.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| arrayToSort | Array | no | The array to sort in-place | Must be a valid Array |
| sortFunction | Function | no | Comparator function receiving two elements | Must be a JavaScript function; return negative/zero/positive int |

**Callback Signature:** sortFunction(a: var, b: var)

**Pitfalls:**
- Returns `false` silently if `arrayToSort` is not an Array or `sortFunction` is not a valid JavaScript function. No error is reported -- the call simply returns `false`.

**Cross References:**
- `$API.Array.sort$`

**Example:**
```javascript:sort-objects-by-property
// Title: Sort an array of objects by a numeric property
var items = [
    {"name": "C", "value": 3},
    {"name": "A", "value": 1},
    {"name": "B", "value": 2}
];

inline function compareByValue(a, b)
{
    return a.value - b.value;
}

Engine.sortWithFunction(items, compareByValue);
Console.print(items[0].name); // "A"
Console.print(items[1].name); // "B"
Console.print(items[2].name); // "C"
```
```json:testMetadata:sort-objects-by-property
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "items[0].name", "value": "A"},
    {"type": "REPL", "expression": "items[1].name", "value": "B"},
    {"type": "REPL", "expression": "items[2].name", "value": "C"}
  ]
}
```

## uncompressJSON

**Signature:** `var uncompressJSON(String b64)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Performs Base64 decoding, zstd decompression, and JSON parsing -- all of which involve heap allocations.
**Minimal Example:** `var obj = Engine.uncompressJSON(b64String);`

**Description:**
Decompresses a Base64-encoded, zstd-compressed string back into a JSON object. This is the inverse of `Engine.compressJSON()`. The method decodes the Base64 string into a memory block, decompresses it with zstd, then parses the resulting string as JSON. Reports a script error if the JSON parsing fails (e.g., if the input is corrupted or was not produced by `compressJSON`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded zstd-compressed JSON string | Must be output from `Engine.compressJSON()` or equivalent zstd+Base64 encoding |

**Pitfalls:**
- Passing an arbitrary Base64 string that was not produced by `compressJSON` (or equivalent zstd compression) will produce a zstd decompression error or a JSON parse error. The JSON parse failure reports a script error, but zstd decompression failure may produce an empty string that then fails JSON parsing with a generic error.

**Cross References:**
- `$API.Engine.compressJSON$`

**Example:**
```javascript:roundtrip-compress-uncompress
// Title: Roundtrip compress and uncompress a JSON object
var original = {"key": "value", "number": 42};
var compressed = Engine.compressJSON(original);
Console.print(typeof compressed); // "string"

var restored = Engine.uncompressJSON(compressed);
Console.print(restored.key);    // "value"
Console.print(restored.number); // 42
```
```json:testMetadata:roundtrip-compress-uncompress
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "restored.key", "value": "value"},
    {"type": "REPL", "expression": "restored.number", "value": 42}
  ]
}
```

## undo

**Signature:** `void undo()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Dispatches to the message thread via `MessageManager::callAsync` for non-script-transaction undo operations. For script transactions (marked with `%SCRIPT_TRANSACTION%`), executes synchronously but still uses the `UndoManager` which involves heap operations.
**Minimal Example:** `Engine.undo();`

**Description:**
Reverts the last undoable action registered with the control undo manager. The behavior depends on the type of the last undo entry: if the undo description is the internal marker `%SCRIPT_TRANSACTION%` (used by `performUndoAction`), the undo executes synchronously on the calling thread. Otherwise, the undo is dispatched asynchronously to the message thread via `MessageManager::callAsync` using a weak reference to the processor for safety. This means non-script undo operations complete after the method returns. The method has no return value and no error reporting if the undo stack is empty -- calling `undo()` with nothing to undo is a silent no-op (JUCE's `UndoManager::undo()` returns false but the result is not checked).

**Parameters:**

None.

**Pitfalls:**
- Non-script undo operations are dispatched asynchronously to the message thread. The undo has not completed when `undo()` returns. Do not read state immediately after calling `undo()` and expect it to reflect the reverted value.

**Cross References:**
- `$API.Engine.redo$`
- `$API.Engine.performUndoAction$`
- `$API.Engine.clearUndoHistory$`
