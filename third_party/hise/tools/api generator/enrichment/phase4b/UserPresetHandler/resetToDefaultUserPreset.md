UserPresetHandler::resetToDefaultUserPreset() -> undefined

Thread safety: WARNING -- triggers full preset load cycle (voice kill, thread dispatch, ValueTree operations)
Loads the default user preset defined in project settings. Triggers the full
preset load lifecycle (pre-callback, voice kill, background load, post-callback).
Bypasses the undo manager even if setUseUndoForPresetLoading is enabled.
Dispatch/mechanics:
  defaultPresetManager->resetToDefault()
    -> loadUserPresetFromValueTree(defaultPreset, useUndoManagerIfEnabled=false)
    -> full preset load sequence on loading thread
Anti-patterns:
  - Throws a script error if no default preset is configured in project settings.
    No query method exists to check beforehand.
Source:
  ScriptExpansion.cpp  resetToDefaultUserPreset()
    -> DefaultPresetManager::resetToDefault()
    -> loadUserPresetFromValueTree()
