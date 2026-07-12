# Script FX - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h` (line 556-668), `hi_scripting/scripting/ScriptProcessorModules.cpp` (line 760-1158)
**Base class:** `MasterEffectProcessor`, `JavascriptProcessor`, `ProcessorWithScriptingContent`

## Signal Path

ScriptFX (JavascriptMasterEffect) is a dual-mode master effect that processes stereo audio through either a scriptnode DSP network or HISEScript callbacks.

**Network mode** (preferred): audio in -> DspNetwork root node process() -> audio out
**Script mode** (legacy): audio in -> processBlock HISEScript callback -> audio out

The module checks `getActiveNetwork()` first. If a network is active and initialised, the network processes the audio. Otherwise, if the processBlock HISEScript callback is non-empty, it falls back to script mode.

When a network is active and extra modulation chains are available (configured via HISE_NUM_SCRIPTNODE_FX_MODS), the processing is chunked via `processChunkedWithModulation` which subdivides the buffer into smaller chunks to apply modulation chain values to network parameters at sub-block granularity.

For multi-channel routing (more than 2 channels), the `renderWholeBuffer` path is used which sets up the correct channel pointers via the routing matrix before processing.

## Gap Answers

### network-loading-mechanism

The module inherits from DspNetwork::Holder. Networks are created/loaded via the `getOrCreate()` method, typically called from the onInit HISEScript callback where the user creates a DspNetwork object. The active network is set via `setActiveNetwork()`. The SlotFX/HotswappableProcessor interface is also available, allowing network swapping at runtime via `setEffect(networkId)`. Networks are stored as XML ValueTree data within the module's state.

### parameter-exposure

When a network is loaded, its parameters are dynamically exposed through `withDynamicParametersFromNetwork()`. The method reads the network's parameter list and appends them to the module's metadata. Parameters are accessed via `getCurrentNetworkParameterHandler()` which delegates to the network's parameter handler when a network is active, or to the content parameter handler (for HISEScript knobs) when no network is loaded.

### complex-data-routing

Complex data types (Tables, SliderPacks, AudioFiles) declared in the scriptnode network are exposed through the ProcessorWithExternalData interface (inherited via JavascriptProcessor -> ProcessorWithScriptingContent). The network creates the required data objects and they become accessible through the standard ExternalData interfaces. The `DisplayBufferSource` interface provides display buffers for UI visualisation (e.g. oscilloscope, FFT displays).

### modulation-chain-support

ScriptFX supports extra modulation chains configured via `HISE_NUM_SCRIPTNODE_FX_MODS` (default: 0). These chains are managed by `ExtraModulatorRuntimeTargetSource` and can be connected to network parameters at runtime. The connection happens via `connectToRuntimeTargets()` which links modulation chain outputs to network parameter inputs. When modulation chains are active, processing uses `processChunkedWithModulation` for sub-block modulation application.

### dual-mode-operation

ScriptFX supports two modes:
1. **Scriptnode mode**: Active when `getActiveNetwork()` returns a valid network. The network processes audio directly.
2. **HISEScript mode**: Active when no network is loaded but the processBlock callback contains code. Audio buffers are passed to the script callback.

The module checks for network first, then falls back to script. Both modes are mutually exclusive - loading a network takes priority.

### script-vs-hardcoded-difference

ScriptFX (JavascriptMasterEffect) includes a HISEScript engine with onInit, prepareToPlay, processBlock, and onControl callbacks. It can operate in either script or network mode. HardcodedMasterFX (HardcodedMasterFX) loads only compiled C++ DSP networks (from the project DLL) with no script engine. ScriptFX is for development (editable networks), HardcodedMasterFX is for exported plugins (compiled networks).

## Processing Chain Detail

1. **Network check**: Check if an active DspNetwork exists and is initialised (negligible)
2. **Connection lock**: Acquire read lock on the network's connection lock (negligible)
3. **Exception check**: Verify network exception handler is OK (negligible)
4. **Modulation processing**: If extra mod chains exist, process in chunks via processChunkedWithModulation. Otherwise process the full buffer through the network root node (cost depends on network)
5. **Fallback**: If no network, execute HISEScript processBlock callback (cost depends on script)

## Modulation Points

Extra modulation chains (when configured) modulate network parameters via the ExtraModulatorRuntimeTargetSource system. The connection between mod chains and parameters is established at runtime when a network is loaded.

## Conditional Behaviour

- **Network vs Script mode**: If getActiveNetwork() is non-null, network mode is used. Otherwise script mode.
- **Multi-channel routing**: If more than 2 channels are routed, renderWholeBuffer uses a multi-channel path with channel index mapping.
- **Bypass reset**: When unbypassed, the network is reset to clear internal state.

## Interface Usage

- **SlotFX / HotswappableProcessor**: Provides setEffect(networkId), getModuleList(), swap() for network loading and hot-swapping.
- **TableProcessor / SliderPackProcessor / AudioSampleProcessor**: Delegate to the DspNetwork's external data system. Tables, SliderPacks, and AudioFiles declared in the network are exposed through these interfaces.
- **DisplayBufferSource**: Provides display buffers (ring buffers, FFT data) for UI visualisation.

## CPU Assessment

- **Baseline**: Cannot be determined statically - depends entirely on the loaded network
- **Framework overhead**: negligible (network dispatch, lock acquisition)
- **Polyphonic**: false (monophonic master effect)

## UI Components

No dedicated FloatingTile. Uses the standard ScriptingEditor for the script/network editing interface.
