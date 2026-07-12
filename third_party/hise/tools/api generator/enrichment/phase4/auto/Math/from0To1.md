Converts a normalised value (0.0-1.0) to a real value within the range defined by a range object. The inverse of `Math.to0To1()`. The range object can use any of three naming conventions:

| Domain | Property names | Notes |
|--------|---------------|-------|
| **scriptnode** | `MinValue`, `MaxValue`, `SkewFactor`, `StepSize`, `Inverted` | Range objects from scriptnode parameters |
| **UI components** | `min`, `max`, `middlePosition`, `stepSize`, `Inverted` | Uses middle position (internally converted to a skew factor with a small overhead) |
| **MIDI automation** | `Start`, `End`, `Skew`, `Interval`, `Inverted` | Range objects from `MidiAutomationHandler` |

For performance-critical code, pass a fix object instead of a plain JSON object. Create one with `Engine.createFixObjectFactory()` and call `.create()` on the factory. A fix object avoids repeated property lookups and is roughly 3x faster than a plain JSON range.

For even better performance when working with UI component ranges that use `middlePosition`, convert to a skew-factor-based range with `Math.skew()` first. The `middlePosition` convention requires an extra logarithm on each call; a pre-computed skew factor eliminates it. Combining both optimisations (skew factor + fix object) can reduce conversion time from around 83 ms to 25 ms per 100,000 calls.

> [!Warning:middlePosition and SkewFactor are not interchangeable] The UI component convention uses `middlePosition` (the real value that maps to 0.5), while the scriptnode convention uses `SkewFactor` (a gamma-like exponent). These are different values for the same curve shape. Mixing them up produces incorrect mappings without any error.
