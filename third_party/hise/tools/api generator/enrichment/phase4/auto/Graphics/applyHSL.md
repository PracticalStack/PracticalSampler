Applies hue, saturation, and lightness adjustments to the current layer. All three parameters are normalised additive offsets, not absolute values - passing `(0.0, 0.0, 0.0)` produces no change.

- **Hue:** 0.0 = no shift, 0.5 = 180-degree shift (opposite side of the colour wheel), 1.0 = full rotation (wraps cyclically)
- **Saturation:** positive increases saturation, negative decreases it
- **Lightness:** positive brightens, negative darkens
