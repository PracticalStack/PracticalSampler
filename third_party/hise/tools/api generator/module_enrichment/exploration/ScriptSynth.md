# Scriptnode Synthesiser - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h` (line 805-925), `hi_scripting/scripting/ScriptProcessorModules.cpp` (line 1862-2206)
**Base class:** `ModulatorSynth`, `JavascriptProcessor`, `ProcessorWithScriptingContent`

## Signal Path

ScriptSynth (JavascriptSynthesiser) is a polyphonic sound generator that renders each voice through a scriptnode DSP network.

Per-voice: noteOn -> startVoice (initialise network voice) -> calculateBlock (network process per voice) -> apply gain modulation -> FX chain -> mix to output

The Voice::calculateBlock method is the core rendering path:
1. On first render after voice start, initialises the voice in the network via voiceData.startVoice()
2. Clears the voice buffer
3. Sets the voice index via DspNetwork::VoiceSetter
4. Processes through the network root node with modulation chain support
5. Applies gain modulation (per-sample or constant)
6. Runs the voice through the effect chain

## Gap Answers

### network-loading-mechanism

Same as ScriptFX: inherits DspNetwork::Holder. Network is created in onInit callback. The voice killer is set during prepareToPlay when a network is active. On restore, if a ScriptnodeVoiceKiller is found in the gain chain, it is used as the voice killer instead.

### voice-rendering-pipeline

Each voice has its own voice buffer. On calculateBlock:
1. If `isVoiceStart` flag is set, the voice is initialised in the network (voiceData.startVoice) and the voice killer is configured
2. Voice buffer is cleared
3. Network root node processes audio with VoiceSetter for the correct voice index
4. Extra modulation chains are applied via processChunkedWithModulation
5. Gain modulation values (from the Gain mod chain) are multiplied into the voice buffer
6. The effect chain renders on the voice

### parameter-exposure

Network parameters are exposed after the fixed ModulatorSynth parameters (Gain=0, Balance=1, VoiceLimit=2, KillFadeTime=3). The parameter offset is `numModulatorSynthParameters` (4). getAttribute/setInternalAttribute delegate to ModulatorSynth for indices < 4, then subtract the offset and forward to the network parameter handler.

### complex-data-routing

Same mechanism as ScriptFX - complex data types from the network are exposed through the ExternalData interfaces inherited from the scripting content system.

### voice-killing

Voice killing works through two mechanisms:
1. The network can set the voice killer directly via `n->setVoiceKiller(synth->vk)` during voice start
2. On state restore, if a ScriptnodeVoiceKiller module is found in the gain modulation chain, it takes over voice killing duties
The ScriptnodeVoiceKiller monitors the network's gate signal and terminates voices when the gate closes.

### modulation-chain-support

ScriptSynth inherits Gain Modulation (chainIndex 1) and Pitch Modulation (chainIndex 2) from ModulatorSynth. Additional chains are created based on HISE_NUM_SCRIPTNODE_SYNTH_MODS (default: 2). These extra chains start at index 2 in the modChains collection (after Gain and Pitch) and are managed by ExtraModulatorRuntimeTargetSource. The pitch chain is also connected to runtime targets via connectToRuntimeTargets.

## Processing Chain Detail

1. **MIDI handling** (preHiseEventCallback): Forward non-noteOn events to the network via voiceData.handleHiseEvent (negligible)
2. **Voice start** (preStartVoice): Mark voice for initialisation on next render (negligible)
3. **Voice render** (Voice::calculateBlock):
   - Voice initialisation in network if isVoiceStart (negligible, once per note)
   - Clear voice buffer (low)
   - Network process with modulation chunks (depends on network)
   - Apply gain modulation (low)
   - Effect chain rendering (depends on effects)

## Modulation Points

- **Gain Modulation** (chainIndex 1): Multiplied into voice buffer after network processing
- **Pitch Modulation** (chainIndex 2): Connected to network runtime targets for pitch control
- **Extra chains**: Connected to network parameters via ExtraModulatorRuntimeTargetSource

## Interface Usage

Same as ScriptFX. All complex data interfaces delegate to the network's external data system.

## CPU Assessment

- **Baseline**: Cannot be determined - depends on loaded network
- **Framework overhead**: negligible per voice (voice setter, buffer clear, gain multiply)
- **Polyphonic**: true - cost scales linearly with active voices

## UI Components

Uses ScriptingEditor for script/network editing.
