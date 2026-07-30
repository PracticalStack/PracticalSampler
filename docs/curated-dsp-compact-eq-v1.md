# Curated Compact EQ v1

`drs.compactEq` v1 is a single stereo tonal band, deliberately not a generic multi-band EQ.
`mode` is a discrete stable choice: `0` low-pass, `1` bell, or `2` high-pass. The remaining
controls are `frequencyHz` (40–18,000 Hz), `q` (0.25–12), `gainDb` (-18–18 dB, bell only), and
`mix` (0–1). All scopes use the same five-control surface and the same no-tail, bounded-state
implementation.

Frequency, Q, gain, and wet/dry updates compile through the existing graph control smoothing and
interpolate normalized RBJ biquad coefficients across the callback. A mode update selects one of
the frozen stable filter designs at the midpoint rather than interpolating between topologies.
Each channel retains only its two input and two output history values. Preparation and reset are
deterministic; processing performs no allocation and clamps non-finite or runaway samples to a
safe finite range.
