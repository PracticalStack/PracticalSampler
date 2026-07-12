# Math -- Project Context

## Project Context

### Real-World Use Cases
- **UI interaction clamping**: Any plugin with draggable controls (XY pads, envelope editors, zoom handlers) uses `Math.range()` to constrain mouse coordinates to normalised [0, 1] bounds before applying values. This is by far the most common Math usage pattern.
- **Perceptual audio scaling**: Plugins with peak meters, compressor displays, or gain controls use `Math.pow()` with fractional exponents (typically 0.25) to map linear gain values to visually proportional meter positions. The inverse power (4.0) converts back.
- **MIDI humanization**: Sequencers and arpeggiators combine `Math.random()` with `Math.pow()` to create biased timing offsets that cluster near zero, producing natural-sounding rhythmic variation.
- **Parameter range conversion**: Plugins with MIDI automation or preset systems pass module parameter metadata (containing `MinValue`, `MaxValue`, `SkewFactor`) directly to `Math.from0To1()` and `Math.to0To1()` for normalised-to-real value conversion.

### Complexity Tiers
1. **Basic arithmetic** (most common): `range`/`clamp`, `abs`, `min`, `max`, `round`, `floor` - used in virtually every script for value clamping, distance checks, and rounding.
2. **Audio-domain math**: `pow`, `log10`, `random`, `fmod`, `sin`/`cos` - used for perceptual scaling curves, step quantization, biased randomization, circular UI layouts, and LFO waveform generation.
3. **Range conversion**: `from0To1`, `to0To1`, `skew` - used for bidirectional mapping between normalised (0-1) and real parameter values with skewed distributions.

### Practical Defaults
- Use `Math.pow(linearGain, 0.25)` to scale linear peak values for visual meter display. The fourth-root curve compresses the upper range and expands the lower range, matching human loudness perception.
- Use `Math.pow(Math.random(), 1.5)` for timing humanization. The exponent biases values toward zero, producing small delays more often than large ones.
- Use `value -= Math.fmod(value, stepSize)` to snap a continuous value to discrete steps (zoom levels, grid divisions).
- When displaying a slider's value, use `parseInt(Math.log10(stepSize) * -1)` to automatically determine the number of decimal places from the step size.

### Integration Patterns
- `Math.from0To1()` / `Math.to0To1()` with `MidiAutomationHandler` range metadata - parameter data objects containing `MinValue`/`MaxValue`/`SkewFactor` can be passed directly as the range argument.
- `Math.pow()` with `Engine.getDecibelsForGainFactor()` / `Engine.getGainFactorForDecibels()` - power curves convert between linear and perceptual domains, then Engine methods convert to dB.
- `Math.range()` inside `ScriptPanel` mouse callbacks - clamp normalised coordinates before calling `changed()` or `setValue()`.
- `Math.sin()` / `Math.cos()` with `Graphics` drawing methods - compute positions for circular LED arrangements, arc indicators, or LFO waveform visualisation.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Math.pow(peak, 0.5)` for meter display | `Math.pow(peak, 0.25)` for meter display | Square root does not compress enough for audio meters. Fourth-root (0.25) better matches the perceptual response needed for peak level visualisation. |
| `x + amount * Math.random()` for bipolar randomization | `x + amount * (2.0 * Math.random() - 1.0)` for bipolar randomization | `Math.random()` returns [0, 1). For bipolar offsets (positive and negative), scale to [-1, 1) first with `2.0 * Math.random() - 1.0`. |
