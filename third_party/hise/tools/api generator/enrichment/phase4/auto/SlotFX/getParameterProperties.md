Returns an array of objects describing the parameters of the slot's underlying processor. Each object contains `text` (parameter name), `defaultValue`, and range properties (`min`, `max`, `skew`, `stepSize`). Primarily useful for HardcodedMasterFX and scriptnode-based slots where you need to discover parameters at runtime.

> [!Warning:Returns undefined for plain SlotFX] Returns `undefined` for a plain SlotFX module, which has no parameters of its own. The parameters belong to the loaded effect, not the slot container.
