# jdsp.jcompressor -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/JuceNodes.h:151-189`
**Base class:** `base::jwrapper<juce::dsp::Compressor<float>, 1>` + `base::jmod<true>`
**Classification:** audio_processor + modulation source, monophonic

## Signal Path

Input audio -> JUCE dsp::Compressor process (AudioBlock) -> Output audio.
Modulation output: `1.0 - getGainReduction()` clamped to 0-1 range, also sent to display buffer.

The jmod base class provides both the display buffer infrastructure and the modulation output mechanism. handleModulation() is called after each process block and returns the normalised gain value.

## Gap Answers

### signal-path-processing

jwrapper::process() creates a juce::dsp::AudioBlock from ProcessData and calls Compressor::process(ProcessContextReplacing). The JUCE Compressor processes all channels together using a single detector -- stereo detection is linked (not independent per channel). The envelope follower tracks the peak level across channels and applies uniform gain reduction.

### modulation-output-value

handleModulation() computes: `v = jlimit<double>(0.0, 1.0, 1.0 - (double)this->objects.get().getGainReduction())`. The output is normalised 0-1 where 1.0 = no gain reduction and 0.0 = maximum gain reduction. This inverted representation means higher values = less compression. The value is also sent to the display buffer via sendModValue(v).

### display-buffer-content

The display buffer receives the same value as the modulation output: the normalised gain level (1.0 - gainReduction). This represents the compressor's gain envelope over time, suitable for visualising compression activity.

### threshold-spelling

The parameter is spelled "Treshold" (without the second 'h') in createParameters(). The C++ setter is correctly named setThreshold(). This is a naming discrepancy in the HISE parameter definition, not a JUCE issue. The parameter ID "Treshold" is the actual identifier that users see and that the serialisation uses.

### monophonic-confirmation

Confirmed monophonic. The jwrapper template is instantiated with NV=1 (hardcoded: `jwrapper<juce::dsp::Compressor<float>, 1>`). No polyphonic_base inheritance. isPolyphonic() returns false.

### no-makeup-gain

Confirmed: there is no makeup gain parameter. The JUCE Compressor class itself does not implement automatic makeup gain. The four parameters (Threshold, Ratio, Attack, Release) are the complete set. Output will be quieter when compression occurs. Users need to add a gain node after the compressor for makeup gain.

### cpu-profile

Per-block processing via AudioBlock. The JUCE Compressor uses a simple envelope follower with attack/release and applies gain reduction. Additional cost from handleModulation() which calls getGainReduction() and sends to display buffer. Low-medium cost overall.

## Parameters

- P=0 Treshold: calls obj.setThreshold(v) -- threshold in dB (-100 to 0)
- P=1 Ratio: calls obj.setRatio(jmax(1.0, v)) -- clamped to minimum 1.0 (no expansion)
- P=2 Attack: calls obj.setAttack(v) -- attack time in ms
- P=3 Release: calls obj.setRelease(v) -- release time in ms

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

## Notes

The Ratio parameter is clamped with jmax(1.0, v) ensuring it cannot go below 1:1, which would be expansion rather than compression. The jmod<true> template parameter (NormalisedModulation=true) means no UseUnnormalisedModulation property is registered.
