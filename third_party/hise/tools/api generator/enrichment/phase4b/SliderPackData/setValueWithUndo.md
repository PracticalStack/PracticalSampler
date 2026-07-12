SliderPackData::setValueWithUndo(int sliderIndex, Double value) -> undefined

Thread safety: UNSAFE -- allocates a SliderPackAction object for the undo manager.
Sets a single slider value with undo support. Behaves like setValue() but registers
the change with the undo manager, storing both old and new values for restoration.
Dispatch/mechanics:
  Creates SliderPackAction(single-value) with old and new value
  -> performs via undo manager -> delegates to setValue()
Pair with:
  setValue -- non-undoable variant for programmatic updates
  setAllValuesWithUndo -- bulk undoable variant
  setAssignIsUndoable -- makes [] operator use this method
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::setValueWithUndo()
    -> creates SliderPackAction -> UndoManager::perform()
