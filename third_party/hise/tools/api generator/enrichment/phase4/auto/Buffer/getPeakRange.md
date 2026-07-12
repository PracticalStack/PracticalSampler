Returns a two-value array `[minValue, maxValue]` for the selected region. This is useful for sparse lane checks before serialising state.

> [!Warning:Check both range values for activity] Do not check only `range[1]` for activity. Negative-only content is valid and only appears in `range[0]`.
