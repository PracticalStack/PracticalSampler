Linearly interpolates between two colours in ARGB space. The blend factor controls the mix: 0.0 returns the first colour, 1.0 returns the second, and values between produce a proportional blend across all four channels. This is the standard way to create hover highlights in LAF paint callbacks - multiply `obj.hover` by a constant blend factor (e.g., 0.25) to avoid branching.

> [!Warning:Blend factor not clamped to 0-1] The blend factor is not clamped to 0.0-1.0. Values outside this range produce extrapolated channel values that may wrap due to byte overflow, yielding unexpected colours.
