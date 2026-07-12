---
title: Math Nodes
factory: math
---

The math factory provides arithmetic and mathematical operations that process audio signals on a per-sample basis. These nodes perform fundamental operations such as multiplication, addition, clipping, and waveshaping. Most are lightweight with negligible CPU cost, making them suitable building blocks for gain staging, signal conditioning, and distortion effects.

All math nodes process each channel independently. Most are polyphonic, meaning each voice can have its own parameter value when used inside a polyphonic context.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.math.abs$]($SN.math.abs$) | Calculates the absolute value of the signal (folds negative values) |
| [$SN.math.add$]($SN.math.add$) | Adds a constant DC offset to the signal |
| [$SN.math.clear$]($SN.math.clear$) | Clears the signal (sets all samples to zero) |
| [$SN.math.clip$]($SN.math.clip$) | Hard-clips the signal to a symmetric range |
| [$SN.math.div$]($SN.math.div$) | Divides the signal by a scalar value |
| [$SN.math.expr$]($SN.math.expr$) | A JIT-compiled math expression using SNEX |
| [$SN.math.fill1$]($SN.math.fill1$) | Fills the signal with a constant 1.0 |
| [$SN.math.fmod$]($SN.math.fmod$) | Computes the floating-point remainder of the signal |
| [$SN.math.intensity$]($SN.math.intensity$) | Controls modulation depth by crossfading between unity and the input |
| [$SN.math.inv$]($SN.math.inv$) | Inverts the phase of the signal |
| [$SN.math.map$]($SN.math.map$) | Maps the signal from one range to another |
| [$SN.math.mod2sig$]($SN.math.mod2sig$) | Converts a 0...1 modulation signal to a -1...1 audio signal |
| [$SN.math.mod_inv$]($SN.math.mod_inv$) | Inverts a modulation signal from 0-1 to 1-0 |
| [$SN.math.mul$]($SN.math.mul$) | Multiplies the signal by a scalar value |
| [$SN.math.neural$]($SN.math.neural$) | Neural network-based audio processing |
| [$SN.math.pack$]($SN.math.pack$) | Processes the signal using a slider pack as a lookup table |
| [$SN.math.pi$]($SN.math.pi$) | Multiplies the signal by pi |
| [$SN.math.pow$]($SN.math.pow$) | Raises the signal to a power using Value as the exponent |
| [$SN.math.rect$]($SN.math.rect$) | Rectifies a normalised signal to 0 or 1 |
| [$SN.math.sig2mod$]($SN.math.sig2mod$) | Converts a -1...1 audio signal to a 0...1 modulation signal |
| [$SN.math.sin$]($SN.math.sin$) | Applies the sine function to the signal |
| [$SN.math.sqrt$]($SN.math.sqrt$) | Applies the square root function to the signal |
| [$SN.math.square$]($SN.math.square$) | Squares the signal (multiplies each sample by itself) |
| [$SN.math.sub$]($SN.math.sub$) | Subtracts a constant value from the signal |
| [$SN.math.table$]($SN.math.table$) | Processes the signal using a table as a lookup function |
| [$SN.math.tanh$]($SN.math.tanh$) | Applies soft saturation using the hyperbolic tangent function |
