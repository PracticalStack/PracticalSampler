ScriptAudioWaveform::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the value through the undo manager, creating an UndoableControlEvent.

Anti-patterns:
  - Do NOT call from onControl callbacks -- intended for user-initiated value
    changes that should be undoable

Pair with:
  setValue -- for non-undoable value changes

Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueWithUndo()
    -> UndoableControlEvent
