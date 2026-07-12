# jdsp.jlinkwitzriley -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/JuceNodes.h:191-257`
**Base class:** `base::jwrapper<juce::dsp::LinkwitzRileyFilter<float>, 1>` + `data::filter_base`
**Classification:** audio_processor, monophonic

## Signal Path

Input audio -> JUCE dsp::LinkwitzRileyFilter process (AudioBlock) -> Output audio.
Filter coefficient display via filter_base.

The jwrapper converts ProcessData to AudioBlock and calls LinkwitzRileyFilter::process(). The filter_base inheritance provides the filter coefficient display mechanism. After each parameter change, sendCoefficientUpdateMessage() notifies the UI to update the filter curve display.

## Gap Answers

### signal-path-processing

jwrapper::process() passes the audio data as a juce::dsp::AudioBlock to LinkwitzRileyFilter::process(ProcessContextReplacing). All channels are filtered together (same cutoff, same type). For processFrame, since LinkwitzRileyFilter has no processSample, jwrapper creates a temporary single-sample AudioBlock.

### type-parameter-values

Confirmed three modes from the C++ source:
- 0 = lowpass (LP) -- `JuceDspType::Type::lowpass`
- 1 = highpass (HP) -- `JuceDspType::Type::highpass`
- 2 = allpass (AP) -- `JuceDspType::Type::allpass`

setParameter<1> casts the double to `(JuceDspType::Type)(int)v`.

### filter-order

JUCE's LinkwitzRileyFilter is a 4th-order (24 dB/octave) crossover filter. It is implemented as a cascade of two 2nd-order sections (biquads). The Linkwitz-Riley design ensures that LP and HP outputs sum to unity gain with flat magnitude response at the crossover frequency.

### filter-display

The node provides filter coefficient display via data::filter_base. getApproximateCoefficients() returns IIR coefficients based on the current type:
- lowpass: IIRCoefficients::makeLowPass(sr, freq, 1.0)
- highpass: IIRCoefficients::makeHighPass(sr, freq, 1.0)
- allpass: IIRCoefficients::makeAllPass(sr, freq, 1.0)

Note: these are approximate 2nd-order coefficients for display purposes, not the actual 4th-order filter coefficients.

### frequency-modulation-stability

The setParameter<0> method validates the frequency value: `if(std::isfinite(v) && v > 20.0 && v < 20000.0)`. This range guard prevents instability from extreme or invalid frequency values. The JUCE LinkwitzRileyFilter implementation uses direct form which is inherently more stable for coefficient updates than the state-variable approach used by the deprecated filters.linkwitzriley.

### monophonic-confirmation

Confirmed monophonic. The jwrapper template is instantiated with NV=1 (hardcoded). No polyphonic_base inheritance. isPolyphonic() returns false.

## Parameters

- P=0 Frequency: validated (isfinite, >20, <20000) then calls setCutoffFrequency(). Also triggers sendCoefficientUpdateMessage() for UI display.
- P=1 Type: cast to enum, calls setType(). Also triggers sendCoefficientUpdateMessage().

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

## Notes

The filter_base inheritance requires setExternalData() to handle FilterCoefficients data type. The prepare() method sets the sample rate on both the jwrapper and the FilterDataObject for correct display. The approximate coefficients returned for display are 2nd-order approximations of the actual 4th-order response.
