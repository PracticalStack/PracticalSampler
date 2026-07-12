Atomically exchanges the loaded effects between this slot and another SlotFX instance. Both slots swap their internal state in a single operation, which is useful for reordering effects in a user-facing FX rack without unloading and reloading.

> [!Warning:Only works with classic SlotFX modules] Only works between classic SlotFX modules. Calling `swap()` on a scriptnode-based slot throws a script error.
