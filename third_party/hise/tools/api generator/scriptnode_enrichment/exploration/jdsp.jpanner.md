# jdsp.jpanner -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/JuceNodes.h:259-295`
**Base class:** `base::jwrapper<juce::dsp::Panner<float>, NV>` + `polyphonic_base`
**Classification:** audio_processor, polyphonic

## Signal Path

Input audio -> JUCE dsp::Panner process (AudioBlock) -> Output audio.

The jwrapper converts ProcessData to AudioBlock and calls Panner::process(). The panner adjusts left/right channel gains based on the Pan position and the selected panning Rule.

## Gap Answers

### signal-path-processing

jwrapper::process() passes audio data as AudioBlock to Panner::process(ProcessContextReplacing). The JUCE Panner applies gain coefficients to left and right channels based on the pan position and rule. For mono input, the JUCE Panner internally handles mono-to-stereo conversion.

### rule-parameter-values

Confirmed seven panning rules from createParameters() which calls setParameterValueNames():
- 0 = Linear (-6dB centre attenuation)
- 1 = Balanced (0dB, both channels at unity when centred)
- 2 = Sine3dB (-3dB constant power)
- 3 = Sine4.5dB (-4.5dB compromise)
- 4 = Sine6dB (-6dB sine variant)
- 5 = Sqrt3dB (-3dB square root constant power)
- 6 = Sqrt4p5dB (-4.5dB square root compromise)

Default is 1 (Balanced). The rule is cast to `juce::dsp::Panner<float>::Rule` via `(Rule)(int)v`.

### mono-input-behaviour

The JUCE Panner class handles mono input internally. When receiving a mono signal (1 channel), it distributes it to stereo output according to the selected panning rule. No explicit mono-to-stereo conversion is needed in the wrapper.

### polyphonic-state

Each voice gets its own Panner instance via `PolyData<Panner, NumVoices> objects` (inherited from jwrapper). The polyphonic_base constructor is called with `(getStaticId(), false)` where `false` means IsProcessingHiseEvent is NOT registered. Each voice can have an independent pan position set via parameter modulation.

### cpu-profile

Per-block processing with simple gain calculations per sample. The CPU cost is negligible for Linear/Balanced rules (multiplication only). Sine and Sqrt rules add transcendental function calls but these are computed per parameter change, not per sample (JUCE caches the gain coefficients). Overall: low cost.

## Parameters

- P=0 Pan: calls setPan(v) -- stereo position from -1.0 (left) to 1.0 (right), default 0.0 (centre)
- P=1 Rule: calls setRule((Rule)(int)v) -- panning law selection, default 1 (Balanced)

## Polyphonic Behaviour

Inherits from polyphonic_base with IsProcessingHiseEvent=false. Each voice has an independent Panner instance via PolyData. Per-voice pan modulation is supported: parameter changes iterate over PolyData and set each voice's Panner independently based on the current voice context.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

The polyphonic_base constructor explicitly passes `false` for addProcessEventFlag, meaning jpanner does not receive MIDI events. The Rule parameter defaults to 1 (Balanced) rather than 0 (Linear), which is the most intuitive default (both channels at unity gain when centred).
