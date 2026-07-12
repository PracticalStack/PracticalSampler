# Hardcoded Envelope Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/hardcoded/HardcodedModules.h`, `HardcodedModules.cpp`, `HardcodedModuleBase.h`, `HardcodedModuleBase.cpp`
**Base class:** `EnvelopeModulator`, `HardcodedSwappableEffect`

## Signal Path

noteOn/noteOff -> [compiled network per voice at control rate] -> modulation output (0-1)

The module runs a compiled C++ network per voice at control rate to produce an envelope modulation signal.

## Gap Answers

### network-loading: How does the envelope modulator load a compiled network?

Same HardcodedSwappableEffect mechanism as other hardcoded modules. The network is loaded via setEffect() from the DLL factory (development) or static factory (export). The OpaqueNode is initialised in polyphonic mode. The network must output a single channel (numChannels == 1 is enforced by checkHardcodedChannelCount).

### voice-lifecycle: How do startVoice/stopVoice/isPlaying interact with the network?

- **startVoice(voiceIndex)**: Calls voiceStack.startVoice() to initialise per-voice state in the compiled network. Then immediately processes one frame (processFrame with a single sample) to get the initial envelope value, which is returned as the voice start modulation value.
- **stopVoice(voiceIndex)**: Empty implementation - note-off handling is delegated to the compiled network via handleHiseEvent/voiceStack.handleHiseEvent.
- **isPlaying(voiceIndex)**: Returns voiceStack.containsVoiceIndex(voiceIndex). The compiled network controls voice lifetime by removing voices from the stack when the envelope finishes.
- **reset(voiceIndex)**: Calls voiceStack.reset(voiceIndex) to clean up per-voice state.
- **calculateBlock**: Sets the voice index via PolyHandler::ScopedVoiceSetter, clears the modulation buffer, creates a ProcessDataDyn pointing to the buffer, and calls opaqueNode->process(). The network fills the buffer with envelope values.

The voice reset mechanism (VoiceResetter interface) allows the compiled network to signal when a voice should be killed (envelope reached zero).

### parameter-exposure: How are parameters exposed alongside Monophonic/Retrigger?

getParameterOffset() returns EnvelopeModulator::Parameters::numParameters (which is 2: Monophonic, Retrigger). Network parameters are accessed at indices starting from this offset.

getAttribute/setAttribute check if the index is below the offset - if so, delegate to EnvelopeModulator base; otherwise subtract offset and call getHardcodedAttribute/setHardcodedAttribute.

getIllegalParameterIds() prevents network parameters from using: Type, Bypassed, ID, Network, Intensity, Monophonic, Retrigger.

### control-rate: Does the envelope run at control rate?

Yes. prepareOpaqueNode sets the block size to largestBlockSize / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR and the sample rate to sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR. This means the compiled network runs at control rate, not audio rate.

### complex-data-exposure: How are complex data types exposed?

Same mechanism as all hardcoded modules via HardcodedSwappableEffect base class.

## Processing Chain Detail

1. **Voice start** (negligible) - initialises per-voice state, processes one frame for initial value
2. **Control rate processing** (depends on network) - compiled network fills modulation buffer at downsampled rate
3. **Buffer apply** (negligible) - modulation values are applied to voice by the EnvelopeModulator base class

## Modulation Points

No additional modulation chains. The compiled network itself IS the modulation source.

## CPU Assessment

- Runs at control rate (downsampled), significantly reducing CPU compared to audio rate
- Per-voice cost depends on network complexity
- Framework overhead: negligible
- Baseline tier: negligible (framework only)

## Notes

- MIDI events are forwarded to the compiled network via handleHiseEvent -> voiceStack.handleHiseEvent. This allows the network to respond to note-off events for release stages.
- The nextEvent member stores the last received HiseEvent so it can be passed to startVoice.
- The compiled network determines voice lifetime - it signals voice death through the VoiceResetter mechanism.
