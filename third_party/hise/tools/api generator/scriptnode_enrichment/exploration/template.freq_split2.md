# template.freq_split2 - Composite Exploration

**Root container:** `container.split`
**Classification:** container (composite template)
**Variant group:** freq_splitN (base variant, N=2)

## Signal Path

The root is a `container.split`, which duplicates the input signal to N parallel band paths and sums their outputs. Each band path is a `container.chain` containing one or more `jdsp.jlinkwitzriley` filters followed by a `math.mul` dummy placeholder.

The jdsp.jlinkwitzriley filter is a JUCE Linkwitz-Riley 4th-order (24 dB/octave) crossover filter. The Type parameter selects: 0=lowpass (LP), 1=highpass (HP), 2=allpass (AP). The Linkwitz-Riley design ensures that LP and HP outputs sum to unity gain with flat magnitude response at the crossover frequency.

### 2-band topology (freq_split2)

Two bands with one crossover point (Band 1):

**band1** (`container.chain`):
- **lr1_1** (`jdsp.jlinkwitzriley`, Type=0 = LP) -- lowpass at Band 1 frequency
- **dummy1** (`math.mul`, Value=1.0) -- user processing placeholder

**band2** (`container.chain`):
- **lr2_1** (`jdsp.jlinkwitzriley`, Type=1 = HP) -- highpass at Band 1 frequency
- **dummy2** (`math.mul`, Value=1.0) -- user processing placeholder

Parameter routing:
- Exposed **Band 1** (20..20000 Hz, log skew) -> lr1_1.Frequency AND lr2_1.Frequency

The LP filter in band1 passes frequencies below the crossover, the HP filter in band2 passes frequencies above. Both receive the same frequency, ensuring phase-coherent crossover. The split container sums the band outputs to reconstruct the full spectrum.

### Filter Type pattern for N-band crossovers

For an N-band crossover with (N-1) crossover points, each band contains (N-1) LR filters. The filter Type for band B at crossover point C follows this pattern:

- If C < B (crossover is below this band): Type=2 (allpass) -- passes all frequencies but matches the phase shift of the corresponding LP/HP pair in adjacent bands
- If C == B: Type=1 (highpass) -- the lower boundary of this band
- If C == B-1: Type=0 (lowpass) -- the upper boundary of this band (only for band1, C index starts at 1)

More precisely, for the 2-band case:
- band1: [LP at crossover 1]
- band2: [HP at crossover 1]

For 3+ bands, interior bands have both HP (from lower crossover) and LP (from upper crossover) plus allpass filters for phase alignment with other crossover points. See the variant explorations for details.

### Phase coherence

The Linkwitz-Riley 4th-order crossover guarantees flat magnitude response when LP and HP outputs are summed. The allpass filters in multi-band variants ensure that all bands experience the same total phase shift at each crossover frequency, maintaining the flat-sum guarantee across the entire spectrum. When the dummy nodes are at Value=1.0 (passthrough), the summed output of all bands equals the input signal with only phase shift applied.

## Gap Answers

### internal-topology

The root is container.split with 2 band children. Each band is container.chain containing [jdsp.jlinkwitzriley, math.mul]. In band1, the LR filter has Type=0 (LP). In band2, the LR filter has Type=1 (HP). These are Linkwitz-Riley 4th-order (24 dB/octave) filters, confirmed by the JUCE LinkwitzRileyFilter implementation which cascades two 2nd-order biquad sections.

### crossover-parameter-routing

Confirmed from the base JSON: Band 1 parameter connects to lr1_1.Frequency (LP filter in band1) and lr2_1.Frequency (HP filter in band2). Both filters receive the same frequency value, creating a matched crossover point. The naming pattern is lrX_Y where X is the band index and Y is the crossover index.

### dummy-mul-purpose

The math.mul nodes (dummy1, dummy2) with Value=1.0 are placeholders for user processing. They serve no functional purpose at default settings (multiply by 1.0 = passthrough). Users should replace or supplement these with their own per-band processing chains (e.g., compression, EQ, saturation applied independently to each frequency band).

### description-accuracy

The base description "Processes each node independently and sums up the output" is the generic container.split description. A more accurate description: "A 2-band frequency crossover using Linkwitz-Riley 4th-order filters with phase-coherent band splitting and summing."

### phase-coherence

Yes, this template achieves flat magnitude response when bands are summed with no user processing. The Linkwitz-Riley crossover design ensures LP + HP = unity at all frequencies (with only phase shift). At the crossover frequency specifically, both LP and HP are at -6dB, and their in-phase sum equals 0dB. The allpass filters in multi-band variants preserve this property.

## Parameters

- **Band 1** (20..20000 Hz, skew=0.23, default ~188 Hz): Crossover frequency between low and high bands. Logarithmic skew provides natural frequency control. Routes to both lr1_1.Frequency and lr2_1.Frequency.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: [{ "parameter": "number of bands (variant)", "impact": "linear", "note": "Each additional band adds LR filters proportional to N-1 crossover points" }]

Each jdsp.jlinkwitzriley is a 4th-order IIR filter (two biquad sections). The 2-band variant has 2 filters total. The N-band variant has N*(N-1) filters total.

## Notes

- Available images: lr4_3.png (shows 3-band variant), split.png
- The skew factor 0.2299 corresponds to logarithmic frequency mapping (approximately log10 scaling across the 20-20000 Hz range).
- This is the base variant of the freq_splitN group. freq_split3 through freq_split5 follow the same pattern with more bands and crossover points.
