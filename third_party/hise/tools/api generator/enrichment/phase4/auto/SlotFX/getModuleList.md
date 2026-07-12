Returns an array of effect type name strings that can be loaded into this slot. For classic SlotFX modules, this is the filtered list of allowed effect types. For HardcodedMasterFX slots, the list might be empty if no compiled DLL is present.

> [!Warning:Returns empty array in exported plugins] For scriptnode-based slots, this method only returns available networks in the HISE IDE. In exported plugins it silently returns an empty array - use hardcoded network names instead of querying at runtime.
