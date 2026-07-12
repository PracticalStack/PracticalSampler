ScriptButton::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the button's value through the undo manager, creating an UndoableControlEvent.
Pass 1 for on or 0 for off.

Anti-patterns:
  - Do NOT call from onControl callbacks -- intended for user-initiated value
    changes that should be undoable

Pair with:
  setValue -- set value without undo support

Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueWithUndo()
    -> creates UndoableControlEvent
