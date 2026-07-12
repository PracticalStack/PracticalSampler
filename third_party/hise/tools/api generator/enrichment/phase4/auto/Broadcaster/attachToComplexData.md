Registers the broadcaster as a source that fires whenever a complex data object changes. The `dataTypeAndEvent` parameter uses `"DataType.EventType"` format, where the data type is one of `"AudioFile"`, `"Table"`, `"SliderPack"`, `"FilterCoefficients"`, or `"DisplayBuffer"`, and the event type controls what triggers the broadcast:

| Event Type | Trigger | Value Argument |
|---|---|---|
| `"Content"` | Data content changes (curve edited, file loaded) | Base64-encoded content string |
| `"Display"` | Display value changes (playback position, table lookup) | Numeric display value |

Pass module IDs and data slot indices as single values or arrays. The total number of data objects monitored is `NumberOfModules * NumberOfIndexes`. For Display events, the value argument is a normalised double (0...1) representing the ruler position, playback position, or last active slider index. Queue mode is automatically enabled when multiple processors or indices are specified.
