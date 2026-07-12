# filters.linkwitzriley -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:170` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:201` (LinkwitzRiley)
**Base class:** `FilterNodeBase<MultiChannelFilter<LinkwitzRiley>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is processed through a fourth-order (LR4, 24 dB/oct) Linkwitz-Riley crossover filter. The implementation computes BOTH a lowpass and highpass path simultaneously using 4th-order IIR coefficients, then selects the output based on Mode.

Per-sample processing (`process(float input, int channel)`):
1. Compute HP output using hpco coefficients and hpData state (4th-order IIR: xm1-xm4, ym1-ym4)
2. Compute LP output using lpco coefficients and lpData state (4th-order IIR)
3. Switch on mode: LP returns lp, HP returns hp, Allpass returns lp + hp

Both LP and HP paths always execute regardless of mode (both state arrays are always updated).

Coefficient computation uses 4th-order Linkwitz-Riley formulas derived from cascaded 2nd-order Butterworth sections. The denominator coefficients (b1co-b4co) are shared between LP and HP; the numerator coefficients differ.

## Gap Answers

### linkwitzriley-mode-values: What do Mode values 0-2 correspond to?

From `LinkwitzRiley::getModes()`: `{ "LP", "HP", "AP" }`.

- 0 = LP (Linkwitz-Riley lowpass, 24 dB/oct)
- 1 = HP (Linkwitz-Riley highpass, 24 dB/oct)
- 2 = AP (Allpass = LP + HP summed, should be unity gain at all frequencies)

### linkwitzriley-order: What is the filter order?

LR4 (4th order, 24 dB/oct). The coefficient computation uses 4th-order polynomials with sqrt(2) factors characteristic of LR4 crossovers. State arrays have 4 delay elements each (xm1-xm4, ym1-ym4).

### linkwitzriley-crossover-use: Is this intended as half of a crossover pair?

Yes. The LP and HP outputs are designed to sum flat (LP + HP = allpass, verified by mode 2). To build a crossover: use two linkwitzriley nodes at the same frequency, one in LP mode and one in HP mode. The outputs will sum to the original signal with only phase shift (no amplitude coloring at the crossover point, which is the defining property of Linkwitz-Riley filters).

### linkwitzriley-q-and-gain-effect: Do Q and Gain affect this filter?

`LinkwitzRiley::updateCoefficients()` has signature `(double sampleRate, double frequency, double /*q*/, double /*gain*/)` -- both Q and Gain are explicitly ignored. LR crossover filters have fixed Q by design (Q = 0.5 per Butterworth section).

### description-accuracy: Confirm characterisation.

Accurate description: "Fourth-order Linkwitz-Riley (LR4) crossover filter with LP, HP, and Allpass modes, 24 dB/oct slope".

## Parameters

- **Frequency:** 20-20000 Hz. Crossover frequency. Smoothed.
- **Q:** 0.3-9.9. **Ignored** -- LR filters have fixed Q.
- **Gain:** -18 to +18 dB. **Ignored.**
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** 0-2 integer. LP, HP, or AP (allpass = LP+HP sum).
- **Enabled:** 0 or 1. Hard bypass.

## Conditional Behaviour

All three modes run the same processing (both LP and HP paths are computed). The mode switch only selects which output is returned. This means mode 2 (AP) has no additional CPU cost vs LP or HP alone.

## Polyphonic Behaviour

Same as all FilterNodeBase nodes.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample 4th-order IIR (8 multiply-adds per sample per channel for LP, plus 8 for HP). Both paths always run even in LP/HP mode, so CPU is constant regardless of mode. Uses a SpinLock around coefficient access which adds minor overhead.

## Notes

The `processSamples()` method has a bug in channel pointer calculation: `buffer.getWritePointer(c + startSample)` should be `buffer.getWritePointer(c, startSample)`. It also ignores the numSamples parameter and uses `buffer.getNumSamples()` instead. However, in the scriptnode usage path via `FilterNodeBase::process()`, startSample is always 0, which mitigates the channel index bug. See issues.md.
