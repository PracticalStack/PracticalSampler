## from0To1

**Examples:**

```javascript:normalised-automation-conversion
// Title: Converting normalised automation values using parameter metadata
// Context: When a plugin stores parameter ranges as JSON objects with
// MinValue/MaxValue/SkewFactor, the same object can be passed directly
// to Math.from0To1() - no manual range extraction needed.

// Parameter metadata from an automation data structure
const var paramRange = {
    "MinValue": 20.0,
    "MaxValue": 20000.0,
    "SkewFactor": 0.3
};

// Convert normalised MIDI CC value to the actual parameter value
inline function ccToParameterValue(normValue, rangeObj)
{
    local realValue = Math.from0To1(normValue, rangeObj);
    return realValue;
}

// Display the value with appropriate formatting
var normalisedInput = 0.5;
var freq = ccToParameterValue(normalisedInput, paramRange);
Console.print(freq); // Skewed frequency value, not linear midpoint
```
```json:testMetadata:normalised-automation-conversion
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "freq > 20.0 && freq < 3000.0", "value": true}
}
```

**Pitfalls:**
- When parameter metadata uses the scriptnode convention (`MinValue`/`MaxValue`/`SkewFactor`), the skew factor value is NOT the same as `middlePosition` from the UI Component convention. A `SkewFactor` of 0.3 and a `middlePosition` of 1000 produce different curves even for the same min/max range.
