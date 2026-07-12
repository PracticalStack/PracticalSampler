# Phase 0: Doxygen XML → Base JSON

**Steps:**
1. `batchCreate.bat` -- Runs Doxygen, copies/renames XML into `xml/selection/`, runs ApiExtractor + BinaryBuilder
2. `python api_enrich.py phase0` -- Parses the XML into base JSON

**Output:** `tools/api generator/enrichment/base/ClassName.json` (one file per class)

---

## What Phase 0 Extracts

Phase 0 is fully mechanical -- no AI involvement. It parses the Doxygen XML output and produces a minimal base JSON for each class containing:

- **Class name** -- from the XML filename (already the friendly scripting name)
- **Category** -- looked up from the hardcoded mapping table (see below)
- **Class description** -- from `<detaileddescription>` on the `<compounddef>`
- **Methods** -- for each public function:
  - Method name
  - Return type (normalized C++ type)
  - Parameter names and types (normalized)
  - Description text from Doxygen `/** */` comments
  - Reconstructed signature string

## What Phase 0 Does NOT Extract

These require C++ source analysis (Phase 1) or manual input (Phase 2/3):

- Constants (`addConstant()` calls in constructor)
- Forced parameter types (`ADD_TYPED_API_METHOD_N` macros)
- VarTypes mapping (C++ types remain as `var`, `int`, `String`, etc.)
- `callScope` (thread safety tier)
- Code examples
- `obtainedVia` (how to get an instance)
- `details` (full architecture analysis)
- Cross-references between methods
- Pitfalls and common mistakes

---

## Base JSON Schema

Each `enrichment/base/ClassName.json` has this structure:

```json
{
  "className": "Console",
  "category": "namespace",
  "description": "A set of handy function to debug the script.",
  "methods": {
    "print": {
      "signature": "void print(var debug)",
      "returnType": "void",
      "description": "Prints a message to the console.",
      "parameters": [
        { "name": "debug", "type": "var" }
      ]
    }
  }
}
```

Parameter types are raw C++ types after normalization (see below). Phase 1 maps these to VarTypes.

---

## Method Filtering

Not all public methods in the Doxygen XML are scripting API methods. Phase 0 applies these filters in order:

### 1. Constructor / Destructor Exclusion

Skip methods with an empty `<type>` element (constructors) or names starting with `~` (destructors).

### 2. Infrastructure Method Name Exclusion

These method names are always internal framework methods, even when they have Doxygen comments:

| Method Name | Reason |
|-------------|--------|
| `getObjectName` | JUCE NamedValueSet framework |
| `getClassName` | JUCE framework |
| `allowIllegalCallsOnAudioThread` | Internal audio thread policy |
| `setDebugLocation` | Internal debug infrastructure |
| `timerCallback` | JUCE Timer override |
| `allowRefCount` | Internal reference counting |
| `isControlCallbackPending` | REST API internal (ScriptComponent) |
| `updateContentPropertyInternal` | Internal property update (ScriptComponent) |

### 3. Empty Description Exclusion

Methods with an empty `<detaileddescription>` are excluded. All scripting API methods have Doxygen comments; infrastructure methods generally do not.

### 4. Non-Scriptable Type Exclusion

Methods where any parameter type or return type contains one of these C++ type substrings are excluded:

| Type Pattern | Example |
|--------------|---------|
| `Component` | `ScriptContentComponent*`, `Component*` |
| `MouseEvent` | `const MouseEvent&` |
| `HiseJavascriptEngine` | `HiseJavascriptEngine*` |
| `ValueTree` | `ValueTree` |
| `DebugableObjectBase` | `DebugableObjectBase*`, `DebugableObjectBase::Location` |
| `NotificationType` | `NotificationType` |
| `KeyPress` | `const KeyPress&` |
| `ZLevelListener` | `ZLevelListener::ZLevel` |
| `ProfileCollection` | `ProfileCollection::ID` |
| `WeakCallbackHolder` | `WeakCallbackHolder::CallableObject*` |
| `SubComponentListener` | `SubComponentListener*` |
| `NativeFunctionArgs` | `const var::NativeFunctionArgs&` |
| `Identifier` | `const Identifier&` |
| `Location` | `DebugableObjectBase::Location&` |

These types only appear in C++ internal methods, not in the scripting API surface.

---

## C++ Type Normalization

Phase 0 normalizes C++ reference types to their base form:

| Raw C++ Type | Normalized |
|-------------|-----------|
| `const String &` | `String` |
| `const var &` | `var` |
| `String &` | `String` |
| `var &` | `var` |

Other types (`var`, `int`, `float`, `double`, `bool`, `String`, `void`) pass through unchanged.

---

## XML Encoding Recovery

Some Doxygen XML files contain invalid characters outside the XML 1.0 range (e.g., a corrupted degree symbol `°` in `Math.xml`). Phase 0 handles this by re-reading the file as bytes, stripping non-ASCII/non-whitespace characters, and re-parsing.

---

## Class-to-Category Mapping

Each of the 91 classes is assigned to one of four categories:

### `namespace` -- Global API Namespaces (17)

Static singletons accessed directly by name in HiseScript.

| Class | Class | Class |
|-------|-------|-------|
| Array | Engine | Server |
| Colours | FileSystem | Settings |
| Console | Math | String |
| Content | Message | Synth |
| Date | ModuleIds | Threads |
|  | Sampler | TransportHandler |

### `object` -- Object Types (55)

Instances obtained via factory methods, module references, or other API calls.

| Class | Class | Class |
|-------|-------|-------|
| AudioFile | GlobalCable | PresetStorage |
| AudioSampleProcessor | GlobalRoutingManager | Rectangle |
| BackgroundTask | Graphics | RoutingMatrix |
| BeatportManager | LorisManager | Sample |
| Broadcaster | MacroHandler | ScriptLookAndFeel |
| Buffer | MarkdownRenderer | ScriptModulationMatrix |
| Builder | MessageHolder | ScriptShader |
| ChildSynth | MidiAutomationHandler | SliderPackData |
| ComplexGroupManager | MidiList | SliderPackProcessor |
| ContainerChild | MidiPlayer | SlotFX |
| DisplayBuffer | MidiProcessor | Table |
| DisplayBufferSource | Modifiers | TableProcessor |
| Download | Modulator | ThreadSafeStorage |
| DspModule | NeuralNetwork | Timer |
| Effect | Path | Unlocker |
| ErrorHandler |  | UnorderedStack |
| Expansion |  | UserPresetHandler |
| ExpansionHandler |  | WavetableController |
| FFT |  |  |
| File |  |  |
| FixObjectArray |  |  |
| FixObjectFactory |  |  |
| FixObjectStack |  |  |

### `component` -- UI Components (14)

Created via `Content.addX()` methods.

| Class | Class |
|-------|-------|
| ScriptAudioWaveform | ScriptPanel |
| ScriptButton | ScriptSlider |
| ScriptComboBox | ScriptSliderPack |
| ScriptDynamicContainer | ScriptTable |
| ScriptFloatingTile | ScriptWebView |
| ScriptImage | ScriptedViewport |
| ScriptLabel | ScriptMultipageDialog |

**Not generated by last Doxygen run:** ModulatorMeter, ScriptedPlotter (would be `component` if present).

### `scriptnode` -- ScriptNode DSP (5)

| Class |
|-------|
| Connection |
| DspNetwork |
| NetworkTest |
| Node |
| Parameter |

---

## Running Phase 0

```
batchCreate.bat > NUL 2>&1
python api_enrich.py phase0
```

The `batchCreate.bat` output is redirected to NUL to avoid polluting the agent's context window. If it fails, re-run without redirection to diagnose.

Expected output from the Python step:
```
Phase 0 complete:
  Classes processed: 91
  Methods extracted: 2035
  Infrastructure methods filtered: 2063
  Output: D:\...\enrichment\base
```
