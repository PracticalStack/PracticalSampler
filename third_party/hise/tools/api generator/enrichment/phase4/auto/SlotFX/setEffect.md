Loads a new effect into the slot by type name and returns a handle to control it. The name must match an entry from `getModuleList()`. If the same effect type is already loaded, the call returns immediately without reloading, so there is no need to guard against redundant calls.

In classic mode the returned handle is an `Effect` object; in scriptnode mode it is a `DspNetwork` object.

> [!Warning:Scriptnode mode clears previous networks] In scriptnode mode, calling `setEffect()` clears all previously loaded networks before loading the new one. Any references to previously loaded `DspNetwork` objects become invalid.
