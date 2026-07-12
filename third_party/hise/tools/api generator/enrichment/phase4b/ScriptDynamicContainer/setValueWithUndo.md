ScriptDynamicContainer::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the container's own value through the undo manager, creating an
UndoableControlEvent.
Anti-patterns:
  - Do NOT call from onControl callbacks -- intended for user-initiated value
    changes that should be undoable.
  - Do NOT pass a String value -- reports a script error.
Pair with:
  setValue -- non-undoable version
Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueWithUndo()
