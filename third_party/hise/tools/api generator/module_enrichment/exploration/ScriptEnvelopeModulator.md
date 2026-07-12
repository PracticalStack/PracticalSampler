# Script Envelope Modulator - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h` (line 441-553), `hi_scripting/scripting/ScriptProcessorModules.cpp` (line 1570-1860)
**Base class:** `EnvelopeModulator`, `JavascriptProcessor`, `ProcessorWithScriptingContent`

## Signal Path

ScriptEnvelopeModulator (JavascriptEnvelopeModulator) is a per-voice envelope modulator that generates modulation values from a scriptnode DSP network.

Per-voice: noteOn -> startVoice (init network voice state) -> calculateBlock (network process at control rate) -> modulation output (0-1 per voice)
noteOff -> stopVoice (mark as ringing off) -> network continues until voice killed

The network operates at **control rate** (sample rate / HISE_EVENT_RASTER), not audio rate. The channel count is set to 1.

## Gap Answers

### network-loading-mechanism

Same as other Script* modules: inherits DspNetwork::Holder. Network created in onInit callback. The module sets itself as voice killer in the constructor via `setVoiceKillerToUse(this)`.

### voice-lifecycle

- **startVoice**: Resets the per-voice state (uptime=0, isPlaying=true, isRingingOff=false), then calls voiceData.startVoice() to initialise the voice in the network
- **stopVoice**: Sets isRingingOff=true on the voice state. The network continues processing - it must produce a gate-off signal to eventually kill the voice
- **reset**: Resets per-voice state (uptime=0, isPlaying=false, isRingingOff=false) and calls voiceData.reset()
- **isPlaying**: Returns the isPlaying flag from the per-voice state

### control-rate-processing

Yes, the envelope processes at control rate. In prepareToPlay, the network is prepared with:
- Sample rate: `getControlRate()` (not the audio sample rate)
- Block size: `samplesPerBlock / HISE_EVENT_RASTER`
- Channel count: 1

This means the network operates on downsampled modulation buffers, not full-rate audio.

### parameter-exposure

Network parameters are exposed after the fixed EnvelopeModulator parameters (Monophonic=0, Retrigger=1). The parameter offset is `EnvelopeModulator::Parameters::numParameters` (2). getAttribute/setInternalAttribute delegate to EnvelopeModulator for indices < 2, then subtract the offset and forward to the network parameter handler.

### complex-data-routing

Same mechanism as other Script* modules.

### voice-killer-interaction

The module sets itself as the voice killer in the constructor (`setVoiceKillerToUse(this)`). It implements the VoiceResetter interface. The `isPlaying()` method returns the per-voice `isPlaying` flag. The network must set this flag to false (via the gate mechanism) to signal voice death. When a voice is no longer playing, the parent synth will kill it during its voice management cycle.

## Processing Chain Detail

1. **Voice start** (startVoice): Reset per-voice state, initialise voice in network (negligible)
2. **Calculate block** (calculateBlock):
   - Set voice index via VoiceSetter (negligible)
   - Clear modulation buffer (negligible)
   - Process through network root node at control rate (depends on network)
3. **Voice stop** (stopVoice): Set isRingingOff flag (negligible)

## Modulation Points

No modulation chains on the module itself. The Monophonic and Retrigger parameters are fixed controls on the EnvelopeModulator base class.

## Conditional Behaviour

- **Monophonic mode**: Inherited from EnvelopeModulator base class. When enabled, only one voice is rendered.
- **Retrigger**: When Monophonic is on, controls whether the envelope restarts on new notes.

## Interface Usage

Same as other Script* modules - all complex data interfaces delegate to the network.

## CPU Assessment

- **Baseline**: Cannot be determined - depends on loaded network
- **Framework advantage**: Operates at control rate, significantly reducing CPU compared to audio-rate processing
- **Polyphonic**: true - cost scales with active voices

## Notes

The module only has onInit and onControl callbacks (no processBlock callback like ScriptTimeVariantModulator). All audio processing is handled by the scriptnode network. There is no HISEScript fallback mode for audio processing.
