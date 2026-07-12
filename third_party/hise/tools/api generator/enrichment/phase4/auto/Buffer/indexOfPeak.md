Finds the index of the highest absolute sample value in the selected region. This is useful for anchor points when aligning events to the strongest transient.

> [!Warning:Keep startSample non-negative] The start index is not fully clamped for negative values. Pass non-negative `startSample` values only.
