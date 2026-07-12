ScriptSliderPack::setAllValuesWithUndo(NotUndefined value) -> undefined

Thread safety: UNSAFE
Bulk slider write with undo integration.

Dispatch/mechanics:
  Uses undo action path for bulk value updates.
  Current implementation forces notify path regardless of callback-suppression flag.

Pair with:
  setAllValues -- non-undo bulk write path
  setAllValueChangeCausesCallback -- related but not fully honored here

Anti-patterns:
  - Do NOT expect callback silence after setAllValueChangeCausesCallback(0) -- undo bulk writes still notify.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::setAllValuesWithUndo() undo bulk path
