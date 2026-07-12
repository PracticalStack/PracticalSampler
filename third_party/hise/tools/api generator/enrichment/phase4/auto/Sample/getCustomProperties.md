Returns a mutable JSON object for attaching arbitrary key-value metadata to this sample. The object is created on first access and reused on subsequent calls. Use this to associate computed data (peak loudness, pitch analysis results, categories) directly with each Sample - the metadata survives sorting and filtering operations, unlike parallel arrays.

> [!Warning:Custom properties are transient] Custom properties are transient - they are not saved to the sample map. The data is lost when the sample map is reloaded or the plugin is closed.
