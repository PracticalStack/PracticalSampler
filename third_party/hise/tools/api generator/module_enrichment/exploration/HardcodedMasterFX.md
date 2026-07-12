# Hardcoded Master FX - C++ Exploration

**Source:** `hi_core/hi_modules/hardcoded/HardcodedModules.h`, `HardcodedModules.cpp`, `HardcodedModuleBase.h`, `HardcodedModuleBase.cpp`
**Base class:** `MasterEffectProcessor`, `HardcodedSwappableEffect`

## Signal Path

Audio in -> [compiled C++ network processing] -> audio out

The module is a framework that hosts a compiled C++ scriptnode network. The audio path is:
1. Silence detection (optional suspension on silence)
2. Set up channel data pointers from routing matrix
3. Process through the compiled network with chunked modulation (extra modulation chains applied)
4. Handle routing matrix display values

## Gap Answers

### network-loading: How does the module load a compiled C++ network?

The module uses a factory-based loading system. In HISE development mode, it loads a DynamicLibraryHostFactory from the project DLL (compiled scriptnode networks). In exported plugins, it uses a static factory (DspNetwork::createStaticFactory()) where the networks are compiled directly into the binary.

The user selects a network from a dropdown in the editor. The dropdown is populated by factory->getModuleList() which returns all available compiled network IDs. Calling setEffect(networkName) initialises a new OpaqueNode from the factory, connects runtime targets, creates complex data listeners, prepares the node, and swaps it in under a write lock.

The "Network" property in the ValueTree stores which network is loaded. On restore, restoreHardcodedData reads this property and calls setEffect().

### parameter-exposure: How are compiled network parameters exposed?

When a network is loaded via setEffect(), the OpaqueNode exposes its parameters through OpaqueNode::ParameterIterator. Each parameter has an info struct with id, range (min/max/interval/skew), and default value. These become the module's attributes with no offset (HardcodedMasterFX has no fixed parameters).

getAttribute/setAttribute delegate to getHardcodedAttribute/setHardcodedAttribute which read/write from a flat float array (lastParameters). When setting, if the parameter has a linked modulation chain, the value is normalised and set as the chain's initial value. Otherwise, the parameter callback is called directly on the OpaqueNode.

Parameter IDs from the network are sanitised (spaces and special characters removed) for use as ValueTree property names.

### modulation-chain-mapping: How do modulation chains connect to compiled network parameters?

The NUM_HARDCODED_FX_MODS preprocessor define controls how many modulation chain slots are created at construction time. These are named "P1 Modulation", "P2 Modulation", etc.

When a network is loaded, its ModulationProperties are read from the OpaqueNode. The ExtraModulatorRuntimeTargetSource maps these modulation slots to network parameters. During processing, processChunkedWithModulation splits the buffer into chunks, evaluates modulation chains for each chunk, and applies the modulated values to network parameters before calling the network's process method.

The modulation chain IDs and colours are updated to match the parameter they modulate when a network is loaded (updateModulationChainIdAndColour).

### complex-data-exposure: How are Tables, SliderPacks, and AudioFiles exposed?

When setEffect() loads a network, it reads numDataObjects from the OpaqueNode for each ExternalData type (Table, SliderPack, AudioFile, DisplayBuffer). For each data object, it creates the corresponding HISE data object (Table, SliderPackData, MultiChannelAudioBuffer, SimpleRingBuffer) via getOrCreate and registers a DataWithListener that synchronises changes between the HISE UI data and the compiled network's internal data.

The data objects are stored in ReferenceCountedArrays (tables, sliderPacks, audioFiles, displayBuffers) and accessed through the standard ProcessorWithExternalData interface. The editor creates appropriate UI editors for each complex data object.

Serialisation writes the data as Base64-encoded strings in the ValueTree, with AudioFiles also storing their sample range.

### compiled-vs-interpreted: How does this differ from ScriptFX?

ScriptFX (and other Script* variants) load scriptnode networks from XML. They interpret the network graph at runtime, creating a tree of individual node objects. HardcodedMasterFX loads a compiled C++ version of the same network from a DLL (development) or static factory (export). The compiled version runs the entire network as a single optimised function call through the OpaqueNode interface, eliminating per-node overhead.

Both share the same network design workflow in HISE's scriptnode editor, but Hardcoded modules require an explicit compilation step (Export > Compile DSP Networks as DLL).

### channel-count: How does multi-channel routing work?

The routing matrix determines which channels are active. checkHardcodedChannelCount() counts the number of connected source channels and compares against the OpaqueNode's numChannels. If they don't match, channelCountMatches is set to false and processing is skipped. The editor shows a "Channel mismatch" error.

For stereo networks (numChannels=2), the standard MasterEffectProcessor rendering is used. For multi-channel networks, applyEffect is called directly on the full buffer.

### performance: What is the CPU overhead?

The framework overhead is negligible: a silence check, a read lock acquisition, channel pointer setup, and modulation chain evaluation (if any chains have modulators). The actual CPU cost depends entirely on the loaded compiled network.

## Processing Chain Detail

1. **Silence detection** (negligible) - checks if buffer is silent for suspension
2. **Channel setup** (negligible) - maps routing matrix channels to buffer pointers
3. **Modulation chain evaluation** (negligible to low) - evaluates extra modulation chains in chunks
4. **Compiled network processing** (depends on network) - calls OpaqueNode::process via RenderData
5. **Display value update** (negligible) - updates routing matrix display

## Modulation Points

Extra modulation chains (P1, P2, ..., up to NUM_HARDCODED_FX_MODS) are mapped to compiled network parameters via ModulationProperties. The mapping is determined by the network itself - parameters that declare modulation connections in their scriptnode definition get linked to the corresponding modulation chain slot.

## Interface Usage

- **SlotFX**: The hot-swap mechanism. getModuleList() returns available compiled networks from the factory. setEffect() loads the selected network.
- **TableProcessor/SliderPackProcessor/AudioSampleProcessor**: Complex data types declared in the compiled network are exposed through these interfaces. The number and type of data objects is determined by the loaded network.
- **RoutingMatrix**: Standard multi-channel routing. Determines which audio channels are sent to the compiled network.

## CPU Assessment

- Framework overhead: negligible
- Actual cost: entirely determined by the loaded compiled network
- Baseline tier: negligible (framework only; actual cost depends on loaded network)

## UI Components

The editor (HardcodedMasterEditor) provides:
- A dropdown selector for choosing the compiled network
- Dynamically created parameter sliders (grouped by page/group tags from the network)
- Complex data editors (table, slider pack, audio file) as needed

## Notes

- If the project DLL is not loaded (development mode), the editor shows "No DLL loaded"
- The "Network" property stores the selected network ID for serialisation
- Parameter IDs that conflict with reserved names (Type, Bypassed, ID, Network) cause an error
- MIDI events are currently NOT forwarded to the compiled network (commented out in handleHiseEvent)
