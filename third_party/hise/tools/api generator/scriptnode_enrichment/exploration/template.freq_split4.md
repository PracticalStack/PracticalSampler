# template.freq_split4 - Composite Exploration (Variant)

**Base variant:** template.freq_split2
**Variant parameter:** number of frequency bands = 4

## Variant-Specific Behaviour

4 bands, 3 crossover parameters (Band 1, Band 2, Band 3). Each band contains 3 LR filters (one per crossover point) plus a math.mul dummy.

### Filter Type assignments

From the base JSON:

**band1** (lowest):
- lr1_1: Type=0 (LP), lr1_2: Type=2 (AP), lr1_3: Type=2 (AP)

**band2** (low-mid):
- lr2_1: Type=1 (HP), lr2_2: Type=0 (LP), lr2_3: Type=2 (AP)

**band3** (high-mid):
- lr3_1: Type=2 (AP), lr3_2: Type=1 (HP), lr3_3: Type=0 (LP)

**band4** (highest):
- lr4_1: Type=2 (AP), lr4_2: Type=2 (AP), lr4_3: Type=1 (HP)

### Pattern

Same pattern as freq_split3 extended to 4 bands. Each band has:
- LP at its upper boundary crossover (if not the highest band)
- HP at its lower boundary crossover (if not the lowest band)
- AP at all other crossover points (for phase alignment)

Band 1: LP at xover1, AP at xover2, AP at xover3
Band 2: HP at xover1, LP at xover2, AP at xover3
Band 3: AP at xover1, HP at xover2, LP at xover3
Band 4: AP at xover1, AP at xover2, HP at xover3

### Parameter routing

- **Band 1** (default ~38 Hz) -> lr1_1, lr2_1, lr3_1, lr4_1 (all .Frequency)
- **Band 2** (default ~391 Hz) -> lr1_2, lr2_2, lr3_2, lr4_2 (all .Frequency)
- **Band 3** (default ~2186 Hz) -> lr1_3, lr2_3, lr3_3, lr4_3 (all .Frequency)

## Gap Answers

### variant-difference

Identical structure to freq_split2 with 4 bands and 3 crossover parameters. Each crossover parameter routes to 4 LR filters (one per band). Interior bands use cascaded HP+LP for bandpass plus AP filters for phase alignment. Total: 12 LR filters.

## CPU Assessment

baseline: medium-low
polyphonic: false
scalingFactors: []

12 total LR filters (4 bands x 3 crossovers). Each is a 4th-order IIR.
