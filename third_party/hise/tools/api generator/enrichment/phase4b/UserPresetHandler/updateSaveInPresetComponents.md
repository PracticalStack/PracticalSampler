UserPresetHandler::updateSaveInPresetComponents(JSON obj) -> undefined

Thread safety: WARNING -- converts DynamicObject to ValueTree, iterates components for type lookup, calls restoreAllControlsFromPreset with value change notifications
Restores UI component values from a JSON object previously created by
createObjectForSaveInPresetComponents. Re-adds the "type" property by looking
up each component by ID, then calls restoreAllControlsFromPreset. Unmatched
component IDs are silently skipped.
Required setup:
  const var uph = Engine.createUserPresetHandler();
  // Typically called inside a custom load callback
Dispatch/mechanics:
  DynamicObject -> ValueTree conversion
    -> for each child: look up component by id -> re-add "type" property
    -> content->restoreAllControlsFromPreset(valueTree)
Pair with:
  createObjectForSaveInPresetComponents -- creates the object to pass here
  setUseCustomUserPresetModel -- typically used within custom load callback
Source:
  ScriptExpansion.cpp  updateSaveInPresetComponents()
    -> ValueTreeConverters::convertDynamicObjectToValueTree()
    -> re-adds "type" per component
    -> content->restoreAllControlsFromPreset()
