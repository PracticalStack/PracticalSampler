SliderPackData::setAllValuesWithUndo(var value) -> undefined

Thread safety: UNSAFE -- allocates an Array<float> and interacts with the undo manager.
Same as setAllValues() but registers the operation with the undo manager. Accepts a
single number, Array, or Buffer. Stores both old and new value arrays for restoration.
Dispatch/mechanics:
  Same as setAllValues() but passes useUndoManager=true
  -> creates SliderPackAction (multi-value) storing old/new arrays
  -> SliderPackData::setFromFloatArray()
Pair with:
  setAllValues -- non-undoable variant
  setValueWithUndo -- single-value undoable variant
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::setAllValuesWithUndo()
    -> same path as setAllValues with undo manager enabled
