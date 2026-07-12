Removes the currently loaded effect and restores the slot to a unity-gain passthrough state. After clearing, the slot passes audio through with minimal overhead. In practice, calling `setEffect("EmptyFX")` achieves the same result and is often preferred in selection logic because it avoids a special case for the "off" state.

> [!Warning:Previous Effect handles become invalid] Any `Effect` handle obtained from a previous `setEffect()` or `getCurrentEffect()` call becomes invalid after clearing. The old effect processor is deleted asynchronously.
