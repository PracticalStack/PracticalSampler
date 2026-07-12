# DspNetwork -- Class Analysis

## Brief
Top-level scriptnode DSP graph container for creating, connecting, and processing audio nodes.

## Purpose
DspNetwork is the central container for a scriptnode DSP graph within HISE. It manages the creation, lookup, and lifecycle of processing nodes organized in a hierarchical tree rooted at a `container.chain` node. The network handles audio processing by delegating to its root node, supports polyphonic voice management, integrates with the HISE parameter system for DAW automation, and provides an undo system for graph editing. It requires the hosting script processor to implement the `DspNetwork::Holder` interface (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator).

### How to Obtain

| Method | Returns | When to Use |
|--------|---------|-------------|
| UI workflow (package icon / XML file selection) | - | Default approach. Sufficient when no script-level interaction with the network is needed. |
| `Engine.createDspNetwork("id")` | `DspNetwork` | Script reference on the same processor - for programmatic node creation, parameter queries, or `setForwardControlsToParameters()`. |
| `Engine.getDspNetworkReference("processorId", "id")` | `DspNetwork` | Access from the main interface script, avoiding complex cross-module script communication. |
| `SlotFX.setEffect("networkName")` | `Effect` | Dynamic swapping between multiple scriptnode networks at runtime via the Effect API. Does not return a DspNetwork directly. |

## Details

### Graph Architecture

A DspNetwork owns a flat list of all node instances and a hierarchical ValueTree that defines the signal flow. The root node (typically `container.chain`) contains child nodes and parameters. Nodes are identified by a `FactoryPath` string (`factory.nodetype`, e.g. `core.gain`, `container.split`) and a unique string ID.

Node creation uses a factory pattern with 13+ registered NodeFactory instances. See `create()`, `createAndAdd()`, and `createFromJSON()` for the full node creation API.

### Bracket Access

DspNetwork implements `AssignableObject`, enabling `network["nodeId"]` syntax to retrieve nodes by ID. Assignment (`network["id"] = value`) is not supported and reports a script error.

### Parameter Forwarding

The `forwardControls` flag (default: true) determines whether UI control values drive network parameters directly or pass through regular script callbacks. See `setForwardControlsToParameters()` for the scripting API. When forwarding is enabled, the `NetworkParameterHandler` bridges root node parameters to the host's `ScriptParameterHandler` interface, making them visible to DAW automation. See `setParameterDataFromJSON()` for batch parameter updates.

### Undo System

An internal `UndoManager` groups operations into transactions every 1500ms via a timer. Undo is enabled by default in the HISE backend (IDE) but disabled in compiled plugins. See `undo()` for the scripting API and its frontend build caveat.

### Processing Safety

Audio processing uses a non-blocking `ScopedTryReadLock` on the connection lock -- if the network is being modified (write lock held), processing is silently skipped rather than blocked. Processing also halts if the `ScriptnodeExceptionHandler` has any pending errors. See `processBlock()` for the scripting entry point and `prepareToPlay()` for initialization.

### Polyphonic Support

The constructor's `isPoly` flag enables voice-aware processing. The `PolyHandler` manages voice indices, and `VoiceSetter`/`NoVoiceSetter` RAII wrappers scope voice activation. In monophonic mode, polyphonic node variants are not created.

### Code Integration (Backend)

DspNetwork embeds two code management systems:
- **FaustManager** -- manages Faust DSP source editing and compilation (runs on sample loading thread via `killVoicesAndCall`)
- **CodeManager** -- manages SNEX C++ JIT source code (guarded by `HISE_INCLUDE_SNEX`)

### deleteIfUnused Note

The `deleteIfUnused` method is not registered via `ADD_API_METHOD` in the constructor and is not callable from HISEScript. It is used internally by `clear()`. See the disabled entry for `deleteIfUnused` in methods.md.

## obtainedVia
`Engine.createDspNetwork(id)` -- requires the script processor to be a `DspNetwork::Holder` (reports error if not).

## minimalObjectToken
nw

## Constants
None. DspNetwork registers zero constants via `addConstant()`.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Engine.createDspNetwork("myNetwork")` in a plain Script Processor | Use a DspNetwork::Holder module (ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator, or ScriptEnvelopeModulator) | `createDspNetwork` requires the processor to implement `DspNetwork::Holder`. Plain Script Processors and Script Voice Start Modulators do not support this. |
| Calling `processBlock` before `prepareToPlay` | Call `prepareToPlay(sampleRate, blockSize)` first | Processing silently returns without output if the network has not been initialized. |
| Passing mismatched buffer sizes to `processBlock` | Ensure all channel buffers have the same sample count | Buffer size mismatch across channels in the array triggers a script error. |

## codeExample
```javascript
// Create a DspNetwork (must be in a Scriptnode-capable processor)
const var nw = Engine.createDspNetwork("MyNetwork");

// Create nodes programmatically
const var gain = nw.create("core.gain", "myGain");
const var osc = nw.create("core.oscillator", "myOsc");

// Access a node by ID (bracket syntax)
const var ref = nw["myGain"];
```

## Alternatives
- `Builder` -- programmatically creates the HISE module tree (synths, effects, modulators); DspNetwork manages the scriptnode DSP graph -- different systems entirely.
- `DspModule` -- legacy API for loading external DSP libraries; DspNetwork is the modern scriptnode graph container.

## Related Preprocessors
`USE_BACKEND`, `HISE_INCLUDE_SNEX`, `HISE_INCLUDE_FAUST`

## Diagrams

### dspnetwork-graph-topology
- **Brief:** DspNetwork Graph Architecture
- **Type:** topology
- **Description:** DspNetwork owns a flat node list and a hierarchical ValueTree. The ValueTree root (type Network) contains a single root Node (container.chain) which contains child Nodes and Parameters. Parameters contain Connections to other nodes. NodeFactory instances (container, core, math, fx, etc.) create nodes by matching the FactoryPath prefix. The NetworkParameterHandler bridges root parameters to the host/DAW. The Holder interface manages multiple networks per processor, with active/debugged network switching.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: DspNetwork methods are graph construction operations (create, get, clear) and audio processing entry points (prepareToPlay, processBlock) with no silent-failure preconditions that could be caught at parse time. The only constraint (processor must be a Holder) is enforced at the Engine.createDspNetwork level, not within DspNetwork methods.
