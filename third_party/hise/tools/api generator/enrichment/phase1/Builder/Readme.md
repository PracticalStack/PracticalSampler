# Builder -- Class Analysis

## Brief
Programmatic module tree construction tool for creating synths, effects, modulators, and MIDI processors at init time.

## Purpose
The `Builder` object enables programmatic construction of the HISE module tree from script. It creates, configures, and destroys sound generators, effects, modulators, and MIDI processors using a build-index addressing system. All module creation is restricted to the `onInit` callback. The Builder is the write-side counterpart to `Synth.get*()` -- it constructs what those methods later retrieve. After all modules are created and configured, `flush()` must be called to update the UI and patch browser.

## Details

### Build Index System

The Builder maintains an internal array of processor references. Each module created or registered gets an array index called a "build index." Index 0 is always the MainSynthChain (pre-populated in the constructor). Subsequent calls to `create()` or `getExisting()` append to this array and return the new index. All other methods (`get`, `setAttributes`, `clearChildren`, `connectToScript`) accept a build index to identify the target module. Build indexes are session-local -- they are meaningful only within a single Builder instance's lifetime and are not persistent.

### Module Creation Flow

See `create()` for the full creation API. The `type` parameter must be one of the type strings from the dynamic constants (`SoundGenerators`, `Modulators`, `Effects`, `MidiProcessors`). The `chainIndex` parameter uses the values from the `ChainIndexes` constant. Creation is idempotent -- existing processors with the same ID are reused.

### clear() and clearChildren()

`clear()` removes ALL modules except the calling script processor. See `clear()` for threading and UI coordination details. `clearChildren()` targets a specific chain on a specific module -- see `clearChildren()` for usage.

### get() Interface Type Dispatch

`get(buildIndex, interfaceType)` returns a typed scripting wrapper. See `get()` for the full list of interface types and failure modes.

### Workflow Pattern

Builder code follows a specific workflow: it is typically commented out by default in the script, uncommented when the module tree needs to be rebuilt, run once, then re-commented. The destructor warns if `flush()` was not called after modifications, enforcing proper cleanup.

## obtainedVia
`Synth.createBuilder()`

## minimalObjectToken
b

## Constants
None. All Builder constants are dynamic (factory-generated at runtime).

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| MidiProcessors | JSON | All registered MIDI processor type names. Keys are cleaned type names (spaces removed), values are the original type strings. |
| Modulators | JSON | All registered modulator type names. Same key/value format as MidiProcessors. |
| SoundGenerators | JSON | All registered sound generator (synth) type names. Same key/value format. |
| Effects | JSON | All registered effect processor type names. Same key/value format. |
| InterfaceTypes | JSON | Script wrapper class names for use with `get()`. Keys and values are identical strings: MidiProcessor, Modulator, Synth, Effect, AudioSampleProcessor, SliderPackProcessor, TableProcessor, Sampler, MidiPlayer, RoutingMatrix, SlotFX. |
| ChainIndexes | JSON | Named chain index constants: Direct=-1, Midi=0, Gain=1, Pitch=2, FX=3, GlobalMod=1. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `b.create(...); // no flush` | `b.create(...); b.flush();` | Forgetting flush() leaves the patch browser and UI out of sync. The destructor will warn. |
| `b.create(...)` in `onNoteOn` | `b.create(...)` in `onInit` | Module creation is restricted to onInit via interfaceCreationAllowed(). Other callbacks will throw an error. |

## codeExample
```javascript
// Builder code -- uncomment to rebuild, then re-comment
const var b = Synth.createBuilder();
b.clear();
var synthIdx = b.create(b.SoundGenerators.SineSynth, "MySine", 0, b.ChainIndexes.Direct);
b.setAttributes(synthIdx, {"OctaveTranspose": 5});
var fxIdx = b.create(b.Effects.SimpleReverb, "MyReverb", synthIdx, b.ChainIndexes.FX);
b.flush();
```

## Alternatives
- `Synth` -- retrieves existing modules by name and controls voice/MIDI processing; Builder creates/destroys modules programmatically

## Related Preprocessors
`USE_BACKEND` (UI coordination in clear(), ScopedBadBabysitter threading bypass).

## Diagrams

### builder-create-flow
- **Brief:** Module Creation and Build Index Flow
- **Type:** topology
- **Description:** Shows the Builder.create() flow: script calls create(type, id, rootBuildIndex, chainIndex). Builder resolves rootBuildIndex to a Processor via createdModules array, checks for existing processor with same ID (returns existing index if found), otherwise delegates to raw::Builder::create() which resolves Chain from parent+chainIndex, looks up FactoryType, creates processor, and adds to chain. New processor is appended to createdModules and its index returned. The diagram shows index 0 as MainSynthChain pre-populated, and subsequent indices growing as create/getExisting are called.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Builder methods operate exclusively in onInit with immediate error reporting for invalid build indexes and types. No silent failures, no timeline dependencies, no deferred state that could produce confusing errors.
