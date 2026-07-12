# template.freq_split3 - Composite Exploration (Variant)

**Base variant:** template.freq_split2
**Variant parameter:** number of frequency bands = 3

## Variant-Specific Behaviour

3 bands, 2 crossover parameters (Band 1, Band 2). Each band contains 2 LR filters (one per crossover point) plus a math.mul dummy.

### Filter Type assignments

Each band has filters for both crossover points. The Type values from the JSON:

**band1** (lowest band):
- lr1_1 (crossover 1): Type=0 (LP) -- lowpass at Band 1
- lr1_2 (crossover 2): Type=2 (AP) -- allpass at Band 2 (phase alignment only)

**band2** (middle band):
- lr2_1 (crossover 1): Type=1 (HP) -- highpass at Band 1 (lower boundary)
- lr2_2 (crossover 2): Type=0 (LP) -- lowpass at Band 2 (upper boundary)

**band3** (highest band):
- lr3_1 (crossover 1): Type=2 (AP) -- allpass at Band 1 (phase alignment only)
- lr3_2 (crossover 2): Type=1 (HP) -- highpass at Band 2

### Pattern explanation

- Band 1 (low): LP at crossover 1, AP at crossover 2. The LP removes everything above the first crossover. The AP matches the phase shift that crossover 2 introduces in the other bands.
- Band 2 (mid): HP at crossover 1, LP at crossover 2. Together these create a bandpass between crossover 1 and crossover 2.
- Band 3 (high): AP at crossover 1, HP at crossover 2. The HP removes everything below the second crossover. The AP matches the phase shift from crossover 1.

### Parameter routing

- **Band 1** (default ~68 Hz) -> lr1_1.Frequency, lr2_1.Frequency, lr3_1.Frequency
- **Band 2** (default 1000 Hz) -> lr1_2.Frequency, lr2_2.Frequency, lr3_2.Frequency

Each crossover parameter routes to all 3 bands' filters at that crossover index, ensuring matched frequency for LP/HP/AP filters at each crossover point.

## Gap Answers

### variant-difference

The structure is identical to freq_split2 except: 3 bands instead of 2, 2 crossover parameters instead of 1, and each band has 2 LR filters (N-1 per band) instead of 1. The middle band achieves bandpass behaviour by combining HP from crossover 1 and LP from crossover 2. Allpass filters are added for phase alignment at crossover points not directly relevant to a given band.

### mid-band-filter-types

The middle band (band2) uses Type=1 (HP) for crossover 1 and Type=0 (LP) for crossover 2. The HP removes frequencies below the lower crossover, and the LP removes frequencies above the upper crossover, creating an effective bandpass response between the two crossover frequencies.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

6 total LR filters (3 bands x 2 crossovers). Each filter is a 4th-order IIR (two biquad sections). Approximately 50% more CPU than freq_split2.
