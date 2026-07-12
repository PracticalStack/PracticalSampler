Returns a JSON object with modulation visualisation data for the specified target, suitable for drawing custom modulation displays on a ScriptPanel. The returned object contains:

- `valueNormalized` - the slider's current normalised value (0-1)
- `scaledValue` - value after scale-mode modulation is applied
- `addValue` - cumulative additive modulation offset
- `modulationActive` - whether any modulation connections are active
- `modMinValue` / `modMaxValue` - modulation range bounds
- `lastModValue` - previous modulation output value

Results are cached after the first call per target.
