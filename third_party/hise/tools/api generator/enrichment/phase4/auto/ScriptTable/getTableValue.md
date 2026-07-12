Looks up the curve output for a normalised input position and returns the interpolated value. This is the main runtime read path for velocity and modulation transfer functions.

> [!Warning:Normalise input to 0-1 range] Input must be normalised to 0.0 to 1.0. Passing raw MIDI values makes the mapping behave almost constant.
