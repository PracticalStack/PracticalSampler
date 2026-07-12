UserPresetHandler::setUseUndoForPresetLoading(Integer shouldUseUndoManager) -> undefined

Thread safety: SAFE
Enables or disables undo support for user preset loading. When enabled, each
preset load is wrapped in UndoableUserPresetLoad and pushed to the control undo
manager. Engine.undo() then restores the previous preset. Consecutive loads
coalesce (keeps first old state, takes last new state). Default: disabled.
Pair with:
  Engine.undo -- triggers undo of the preset load
  resetToDefaultUserPreset -- bypasses undo even when enabled
Source:
  ScriptExpansion.cpp  setUseUndoForPresetLoading()
    -> sets useUndoForPresetLoads flag
  UserPresetHandler.cpp  loadUserPresetFromValueTree()
    -> if flag set: creates UndoableUserPresetLoad -> undoManager.perform()
