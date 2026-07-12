ScriptedViewport::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets the value through the undo manager, creating an UndoableControlEvent.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setValueWithUndo(2);
Pair with: setValue (sets value without undo support)
Anti-patterns: Do NOT call from onControl callbacks -- intended for user-initiated value changes that should be undoable.
Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueWithUndo() -> UndoableControlEvent -> UndoManager::perform()
