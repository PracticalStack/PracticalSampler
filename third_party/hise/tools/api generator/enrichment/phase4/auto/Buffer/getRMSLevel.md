Returns RMS level for a selected range, which is useful for average-energy metering rather than peak detection. Use this when you care about perceived signal energy across a window.

> [!Warning:Keep startSample non-negative] The start index is not fully clamped for negative values. Keep `startSample` at `0` or above.
