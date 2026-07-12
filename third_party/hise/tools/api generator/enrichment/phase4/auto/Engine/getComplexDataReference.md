Returns a scripting reference to a complex data object (Table, SliderPack, AudioFile, or DisplayBuffer) owned by another module. This allows cross-module access - for example, reading a table curve from a different processor. The module is looked up by processor ID and the data slot is selected by a zero-based index.

| Data Type | Returns |
|-----------|---------|
| `"Table"` | ScriptTableData reference |
| `"SliderPack"` | ScriptSliderPackData reference |
| `"AudioFile"` | ScriptAudioFile reference |
| `"DisplayBuffer"` | ScriptRingBuffer reference |

> [!Warning:Invalid slot returns undefined silently] If the data slot index exceeds the number of slots the module provides, the method silently returns `undefined` with no error. Check the return value with `isDefined()` before use.