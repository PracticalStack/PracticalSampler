# filters.ring_mod -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FilterNode.h:165` (template alias), `hi_dsp_library/dsp_basics/MultiChannelFilters.h:379` (RingmodFilterSubType)
**Base class:** `FilterNodeBase<MultiChannelFilter<RingmodFilterSubType>, NV>` -> `data::filter_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Input audio is multiplied by an internal sine oscillator, producing ring modulation (sum and difference frequencies). The processing is a wet/dry mix controlled by Q:

Per-sample processing:

```
invGain = 1.0 - oscGain
oscValue = oscGain * sin(uptime)
output = invGain * input + input * oscValue
       = input * (invGain + oscGain * sin(uptime))
       = input * (1 - oscGain + oscGain * sin(uptime))
uptime += uptimeDelta
```

Where `uptimeDelta = frequency / sampleRate * 2 * pi` and `oscGain = jmap(Q, 0.3, 9.9, 0.0, 1.0)`.

At Q=0.3 (minimum), oscGain=0: output = input (dry signal only, no modulation).
At Q=9.9 (maximum), oscGain=1: output = input * sin(uptime) (pure ring modulation).
Intermediate Q values crossfade between dry and ring-modulated signal.

The oscillator phase (`uptime`) is shared across all channels, so all channels are modulated in phase.

## Gap Answers

### ring-mod-signal-path: How does the ring modulator work?

Classic ring modulation: input multiplied by a sine oscillator at the Frequency parameter. The Q parameter controls wet/dry mix (not resonance). See Signal Path above.

### ring-mod-mode-values: What do the modes represent?

From `RingmodFilterSubType::getModes()`: `{ "RingMod" }`. Only one mode. `setType(int /*t*/)` is empty. The Mode parameter has no effect.

### ring-mod-q-and-gain-effect: Do Q and Gain affect the ring modulator?

**Q:** Yes, but not as resonance. Q controls the modulation depth (wet/dry mix): `oscGain = jmap(Q, 0.3, 9.9, 0.0, 1.0)`. At minimum Q, no modulation occurs. At maximum Q, full ring modulation.

**Gain:** `updateCoefficients()` signature is `(double sampleRate, double frequency, double q, double /*gain*/)` -- Gain is explicitly ignored.

### ring-mod-filter-coefficients: What does the filter display show?

The `getCoefficientTypeList()` returns `{ FilterHelpers::AllPass }`, and `getCoefficients()` returns empty data. The filter display shows a flat allpass curve, which is technically correct (the ring modulator does not have a traditional frequency response). The display is not particularly useful for this node.

### ring-mod-factory-placement: Why is ring_mod in the filters factory?

It uses the same `FilterNodeBase` infrastructure as all other filter nodes, sharing the parameter interface (Frequency, Q, Gain, Smoothing, Mode, Enabled). This is an architectural convenience -- the node fits the template pattern even though it is functionally an effect rather than a filter. The Frequency parameter drives the oscillator frequency instead of a filter cutoff, and Q drives modulation depth instead of resonance.

## Parameters

- **Frequency:** 20-20000 Hz. Internal sine oscillator frequency. Smoothed.
- **Q:** 0.3-9.9. Modulation depth / wet-dry mix (0.0 to 1.0 internal). Low Q = dry, high Q = full ring mod. Smoothed.
- **Gain:** -18 to +18 dB. **Ignored.**
- **Smoothing:** 0-1 seconds. Coefficient interpolation time.
- **Mode:** Single mode (RingMod). Parameter exists but only one value is valid.
- **Enabled:** 0 or 1. Hard bypass.

## Polyphonic Behaviour

Same as all FilterNodeBase nodes. Note: the oscillator state (`uptime`, `uptimeDelta`) is stored per-voice in the `RingmodFilterSubType` instance within the `PolyData`. However, the oscillator phase is NOT reset on note-on (only `reset()` sets `uptime=0`), so the modulation phase may differ between voices depending on when they started.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample processing with one `sin()` call per sample (shared across channels). The `sin()` is the most expensive operation but is a single call per sample, not per channel.

## Notes

Ring modulation creates inharmonic frequency content (sum and difference of input and oscillator frequencies). Unlike amplitude modulation, it does not preserve the original carrier frequency. At low oscillator frequencies (e.g., 1-5 Hz, though the parameter range starts at 20 Hz), it creates a tremolo effect. The minimum frequency of 20 Hz means true tremolo is not achievable with this node.
