Truncates the value toward zero by removing the decimal part. Always returns an integer (unlike `Math.floor()` which returns a double).

> [!Warning:Differs from floor for negative values] `Math.trunc(-2.7)` returns -2 (toward zero), while `Math.floor(-2.7)` returns -3.0 (toward negative infinity). Choose based on whether you want to round toward zero or toward the lower integer.
