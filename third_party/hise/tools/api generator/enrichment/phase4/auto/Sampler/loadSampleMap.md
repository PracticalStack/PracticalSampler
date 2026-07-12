Loads a sample map by its pool reference string. The reference should match the format returned by `Sampler.getSampleMapList()`. Use a timer to defer the load when calling from a UI callback to avoid audio glitches.

> [!Warning:Parse ComboBox float values to int] ComboBox values arrive as floats (e.g. `1.0`). Always use `parseInt(value - 1)` when converting to an array index for the sample map list.
