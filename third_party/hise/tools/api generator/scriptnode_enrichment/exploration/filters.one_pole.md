# filters.one_pole -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:164` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:337` (SimpleOnePoleSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<SimpleOnePoleSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a first-order (one-pole) IIR filter. The filter maintains one state variable per channel (`lastValues[c]`). Coefficients `a0` and `b1` are computed from frequency as:

```
x = exp(-2*pi*frequency/sampleRate)
a0 = 1 - x
b1 = -x
```

**LP mode:** `output = a0 * input - b1 * lastValue` (standard one-pole lowpass)
**HP mode:** `temp = a0 * input - b1 * lastValue; output = input - temp` (highpass = input minus lowpass)

This gives a 6 dB/octave (first-order) rolloff.

## Gap Answers

### one-pole-mode-values: What do Mode values 0-1 correspond to?

From `SimpleOnePoleSubType::getModes()`: `{ "LP", "HP" }`.

- 0 = LP (Low Pass, 6 dB/oct)
- 1 = HP (High Pass, 6 dB/oct)

### one-pole-q-and-gain-effect: Do Q and Gain affect the filter?

`SimpleOnePoleSubType::updateCoefficients()` has signature `(double sampleRate, double frequency, double /*q*/, double /*gain*/)` -- both Q and Gain are explicitly unnamed/ignored. These parameters have **no effect** on the one_pole filter. They exist only for interface consistency with FilterNodeBase's shared parameter set.

### one-pole-slope: Confirm 6 dB/octave rolloff.

Yes, this is a standard first-order IIR filter with 6 dB/octave rolloff. Implementation: `y[n] = a0*x[n] - b1*y[n-1]` where b1 = -exp(-2*pi*f/sr). This is the classic one-pole topology.

### description-accuracy: Confirm characterisation.

Accurate description: "First-order (one-pole) filter with LP and HP modes, 6 dB/octave slope".

## Parameters

- **Frequency:** 20-20000 Hz. Cutoff frequency. Smoothed.
- **Q:** 0.3-9.9. **Ignored** -- one-pole filters have no resonance parameter.
- **Gain:** -18 to +18 dB. **Ignored** -- one-pole filters have no gain parameter.
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** 0-1 integer. LP or HP.
- **Enabled:** 0 or 1. Hard bypass.

## Conditional Behaviour

Mode 0 (LP) and Mode 1 (HP) use different processing paths in processSamples/processFrame (switch statement). HP is computed as input minus LP output.

## Polyphonic Behaviour

Same as all FilterNodeBase nodes.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

Simplest filter in the factory. One multiply-add per sample per channel.

## Notes

The one_pole filter provides a custom `getCoefficients()` implementation with a custom plot function (`getPlotValue`) for the filter display, since it cannot use standard IIR coefficient visualization. The display curve is computed analytically.

Useful for DC blocking (HP mode), gentle tone shaping, or control signal smoothing.
