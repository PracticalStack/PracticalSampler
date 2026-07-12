# template.freq_split5 - Composite Exploration (Variant)

**Base variant:** template.freq_split2
**Variant parameter:** number of frequency bands = 5

## Variant-Specific Behaviour

5 bands (maximum variant), 4 crossover parameters (Band 1 through Band 4). Each band contains 4 LR filters (one per crossover point) plus a math.mul dummy.

### Filter Type assignments

From the base JSON:

**band1** (lowest):
- lr1_1: Type=0 (LP), lr1_2: Type=2 (AP), lr1_3: Type=2 (AP), lr1_4: Type=2 (AP)

**band2**:
- lr2_1: Type=1 (HP), lr2_2: Type=0 (LP), lr2_3: Type=2 (AP), lr2_4: Type=2 (AP)

**band3** (middle):
- lr3_1: Type=2 (AP), lr3_2: Type=1 (HP), lr3_3: Type=0 (LP), lr3_4: Type=2 (AP)

**band4**:
- lr4_1: Type=2 (AP), lr4_2: Type=2 (AP), lr4_3: Type=1 (HP), lr4_4: Type=0 (LP)

**band5** (highest):
- lr5_1: Type=2 (AP), lr5_2: Type=2 (AP), lr5_3: Type=2 (AP), lr5_4: Type=1 (HP)

### Pattern

Consistent with freq_split2/3/4. Each band B (1-indexed):
- Filter at crossover C where C == B-1: Type=0 (LP, upper boundary)
- Filter at crossover C where C == B: Type=1 (HP, lower boundary)
- All other crossover filters: Type=2 (AP, phase alignment)

Exception: band1 has no HP (it is the lowest), band5 has no LP (it is the highest).

### Parameter routing

- **Band 1** (default ~28 Hz) -> lr1_1, lr2_1, lr3_1, lr4_1, lr5_1 (all .Frequency)
- **Band 2** (default ~188 Hz) -> lr1_2, lr2_2, lr3_2, lr4_2, lr5_2 (all .Frequency)
- **Band 3** (default 1000 Hz) -> lr1_3, lr2_3, lr3_3, lr4_3, lr5_3 (all .Frequency)
- **Band 4** (default ~3445 Hz) -> lr1_4, lr2_4, lr3_4, lr4_4, lr5_4 (all .Frequency)

Default crossover frequencies are spaced roughly logarithmically across 20-20000 Hz.

## Gap Answers

### variant-difference

Identical structure to freq_split2 with 5 bands and 4 crossover parameters. Each crossover parameter routes to 5 LR filters. Total: 20 LR filters. This is the largest freq_split variant.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []

20 total LR filters (5 bands x 4 crossovers). Each is a 4th-order IIR (two biquad sections). This is the most CPU-intensive freq_split variant. The allpass filters are necessary for phase coherence but add significant overhead compared to a naive crossover design.
