Returns a handle to the effect currently loaded in the slot. Use this to control the loaded effect's parameters or bypass state after loading.

> [!Warning:Return type depends on slot mode] The return type depends on the slot mode: an `Effect` handle for classic SlotFX modules, or a `DspNetwork` object for scriptnode-based slots. Code that assumes one type will fail on the other.
