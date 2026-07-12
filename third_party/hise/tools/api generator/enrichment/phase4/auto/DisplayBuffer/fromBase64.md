Restores the display buffer state from a base64-encoded string previously exported with `toBase64()`. When `useUndoManager` is true, the operation supports undo/redo through the control undo manager.

> [!Warning:Only supported by stateful buffer types] Most buffer types (FFT, oscilloscope, goniometer) do not support state serialisation. Calling this method on an unsupported buffer type silently does nothing - there is no error message to distinguish "restored empty state" from "operation not supported".
