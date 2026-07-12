ScriptSlider::setValueWithUndo(NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets slider value through undo manager by creating an undoable control event.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  setValue -- direct write path when undo integration is not required
  changed -- callback/listener flow after scripted edits

Anti-patterns:
  - Do NOT call from onControl callbacks -- can create recursive undo/control behavior.

Source:
  ScriptingApiContent.cpp:2054  undoable control event creation path
