Returns the skew factor for a range where the given mid-point maps to 0.5 in normalised space. The returned value can be used as the `SkewFactor` property in range objects passed to `Math.from0To1()` and `Math.to0To1()`.

The most practical use is converting UI component ranges (which use `middlePosition`) to skew-factor-based ranges for better performance. The `middlePosition` convention requires an extra logarithm calculation on every conversion call; pre-computing the skew factor with `Math.skew()` eliminates that overhead. Combined with a fix object, this can reduce `from0To1` conversion time from around 83 ms to 25 ms per 100,000 calls:

```javascript
// Convert a middlePosition range to a skewFactor range
var skewRange = {
    "MinValue": 20.0,
    "MaxValue": 20000.0,
    "SkewFactor": Math.skew(20.0, 20000.0, 1000.0)
};

// Wrap in a fix object for maximum performance
const var factory = Engine.createFixObjectFactory(skewRange);
const var range = factory.create();
var value = Math.from0To1(0.5, range);
```
