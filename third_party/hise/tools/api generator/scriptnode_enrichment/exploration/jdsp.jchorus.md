# jdsp.jchorus -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/JuceNodes.h:413-435`
**Base class:** `base::jwrapper<juce::dsp::Chorus<float>, 1>`
**Classification:** audio_processor, monophonic

## Signal Path

Input audio -> JUCE dsp::Chorus process (AudioBlock) -> Output audio.

The jwrapper base class converts ProcessData to a juce::dsp::AudioBlock and calls Chorus::process(). For frame processing, since Chorus has no processSample method, jwrapper creates a temporary single-sample AudioBlock and calls process() on it.

The chorus effect internally uses an LFO-modulated delay line with feedback and wet/dry mixing.

## Gap Answers

### signal-path-processing

jwrapper::process() creates a juce::dsp::AudioBlock from data.getRawChannelPointers() and calls the JUCE Chorus::process() with a ProcessContextReplacing. Stereo channels are handled natively by the JUCE AudioBlock -- all channels are processed together. For processFrame(), a temporary AudioBlock is created from pointer stack allocation since Chorus lacks processSample().

### feedback-negative-values

Feedback is passed directly to JUCE's Chorus::setFeedback(v) without any transformation. In JUCE's Chorus implementation, negative feedback inverts the delayed signal phase before feeding it back into the delay line, creating a flanging-type effect rather than a reinforcing effect.

### missing-description

Confirmed: this is a direct wrapper of juce::dsp::Chorus<float> with NV=1. No getDescription() override exists, so the base data description is empty. An appropriate description would be: "A chorus effect that wraps the JUCE dsp::Chorus class."

### monophonic-confirmation

Confirmed monophonic. The template is instantiated with NV=1 (hardcoded in the struct definition: `jwrapper<juce::dsp::Chorus<float>, 1>`). jwrapper::isPolyphonic() returns false when NumVoices == 1. There is no polyphonic_base inheritance.

### cpu-profile

Per-block processing via AudioBlock. The JUCE Chorus internally maintains an LFO and modulated delay line. Cost is dominated by the delay buffer read/write and LFO computation per sample. Medium cost overall.

## Parameters

- P=0 CentreDelay: calls obj.setCentreDelay(jmin(v, 99.9)) -- clamped to just under 100ms to avoid edge case
- P=1 Depth: calls obj.setDepth(v) -- direct pass-through
- P=2 Feedback: calls obj.setFeedback(v) -- direct pass-through, negative values invert phase
- P=3 Rate: calls obj.setRate(v) -- LFO frequency in Hz
- P=4 Mix: calls obj.setMix(v) -- 0 = fully dry, 1 = fully wet

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []
