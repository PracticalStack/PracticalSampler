DisplayBuffer::fromBase64(String b64, Integer useUndoManager) -> undefined

Thread safety: UNSAFE -- string parameter, potential UndoableAction allocation and undo manager transaction when useUndoManager is true.
Restores the display buffer state from a base64-encoded string. Only buffer types
that implement state serialization (e.g., flex AHDSR envelopes) produce meaningful
results. When useUndoManager is true, wraps the operation in an UndoableAction for
undo/redo support.
Pair with:
  toBase64 -- export state before restoring, or for save/load workflows
Anti-patterns:
  - Do NOT assume all buffer types support fromBase64 -- most types (FFT,
    oscilloscope, goniometer, generic) silently do nothing. Only envelope types
    (flex AHDSR) implement state serialization.
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::fromBase64()
    -> PropertyObject::restoreFromBase64(b64)
    -> when useUndoManager: wraps in B64Action UndoableAction
       -> MainController::getControlUndoManager()->perform()
