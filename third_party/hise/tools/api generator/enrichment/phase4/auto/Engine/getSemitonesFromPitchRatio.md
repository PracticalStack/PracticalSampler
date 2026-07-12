Converts a pitch ratio to cents (not semitones, despite the method name) using the formula `1200 * log2(ratio)`. A ratio of 2.0 returns 1200.0 (one octave), not 12.0. Divide the result by 100 to get semitones.

> [!Warning:Returns cents, not semitones] This method returns cents, not semitones. It is not a true inverse of `Engine.getPitchRatioFromSemitones()`, which expects semitones as input.