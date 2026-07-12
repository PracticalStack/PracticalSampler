Returns the complete MIDI automation configuration as an array of JSON objects, one per CC-to-parameter mapping. The returned array is a snapshot - modifying its entries does not affect the live automation state. Use `setAutomationDataFromObject()` to write changes back.

Each object in the array contains these properties:

| Property | Type | Description |
| --- | --- | --- |
| `Controller` | int | CC number (0-127). |
| `Channel` | int | MIDI channel (1-based, 1-16). `-1` for omni (all channels). |
| `Processor` | String | ID of the target processor (typically `"Interface"`). |
| `Attribute` | String | Parameter ID or custom automation slot ID. |
| `MacroIndex` | int | Macro slot index, or `-1` for direct mapping. |
| `Start` | double | Active sweep range start. |
| `End` | double | Active sweep range end. |
| `FullStart` | double | Lower limit of the settable range. |
| `FullEnd` | double | Upper limit of the settable range. |
| `Skew` | double | Logarithmic skew factor for the range curve. |
| `Interval` | double | Step size (e.g. `1.0` for discrete controls). |
| `Inverted` | bool | Whether the CC-to-parameter mapping is inverted. |
| `Converter` | String | Encoded value-to-text converter (Base64). |
