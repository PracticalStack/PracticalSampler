Converts a real value within a range to a normalised value (0.0-1.0). The inverse of `Math.from0To1()`. Accepts the same three range object conventions (scriptnode, UI component, MIDI automation) and the same fix object optimisation.

> [!Warning:middlePosition and SkewFactor are not interchangeable] The UI component convention uses `middlePosition` (the real value that maps to 0.5), while the scriptnode convention uses `SkewFactor` (a gamma-like exponent). Mixing them up produces incorrect mappings without any error.
