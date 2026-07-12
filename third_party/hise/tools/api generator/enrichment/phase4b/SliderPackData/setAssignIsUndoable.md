SliderPackData::setAssignIsUndoable(bool shouldBeUndoable) -> undefined

Thread safety: SAFE -- sets an internal boolean flag without any allocations or locks.
When enabled, [] operator assignments (e.g., spd[3] = 0.75) go through the undo system
via setValueWithUndo() instead of setValue(). Disabled by default.
Pair with:
  setValueWithUndo -- the undo path used when this is enabled
  setValue -- the non-undo path used when this is disabled (default)
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::setAssignIsUndoable()
    -> sets assignIsUndoable flag
    -> assign() override checks flag: if true, calls setValueWithUndo(); else setValue()
