Wraps the value into the range [0, limit) so it always stays positive and cycles. Computed as `fmod(value + limit, limit)`. Useful for cyclic parameters like phase angles, circular buffer indices, or looping animation frames.

The key difference from `Math.fmod()` is the handling of negative values:

```javascript
Math.fmod(-1.0, 4.0);  // -1.0 (sign of the dividend)
Math.wrap(-1.0, 4.0);  //  3.0 (always positive)
```

Use `Math.wrap()` when you need the result to stay in [0, limit) regardless of input sign. Use `Math.fmod()` when you need the standard remainder with sign preservation.
