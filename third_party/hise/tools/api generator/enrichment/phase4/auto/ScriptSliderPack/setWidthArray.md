Sets non-uniform slider widths using normalised cumulative breakpoints.

The array should describe segment boundaries from `0.0` to `1.0` and have length `getNumSliders() + 1`.

> [!Warning:Array must be numSliders plus one] If the array length does not match `numSliders + 1`, an error is logged and layout falls back to equal widths.

> [!Warning:Update amount and widths together] Update `sliderAmount` and width array together in one code path to keep drawing and hit-testing aligned.
