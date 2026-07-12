Returns the current draggable filter configuration as a JSON object. Use this to inspect the default configuration before customising it with `Effect.setDraggableFilterData()`. See that method for the full property schema.

> [!Warning:Returns undefined for unsupported effects] Silently returns `undefined` for effects that do not support filter visualisation (most built-in effects). Check the return value before using it.
