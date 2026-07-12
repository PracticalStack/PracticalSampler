Returns an array of connection objects representing all active macro-to-parameter mappings across every macro slot. Each element has this shape:

```json
{
  "MacroIndex": 0,
  "Processor": "Simple Gain1",
  "Attribute": "Gain",
  "CustomAutomation": false,
  "FullStart": 0.0,
  "FullEnd": 1.0,
  "Start": 0.0,
  "End": 1.0,
  "Interval": 0.01,
  "Skew": 1.0,
  "Inverted": false
}
```

Use this as the starting point for a read-modify-write cycle with `setMacroDataFromObject()`.

> [!Warning:Returns snapshot, not live reference] The returned array is a snapshot, not a live reference. Pushing, removing, or editing entries in the array has no effect on the actual macro connections until you call `setMacroDataFromObject()` with the modified array.
