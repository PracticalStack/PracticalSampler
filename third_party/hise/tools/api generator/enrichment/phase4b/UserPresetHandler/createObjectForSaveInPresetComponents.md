UserPresetHandler::createObjectForSaveInPresetComponents() -> JSON

Thread safety: WARNING -- exports ValueTree, converts to DynamicObject with heap allocations and string operations
Exports current values of all UI components with saveInPreset enabled.
Returns a JSON object derived from the internal ValueTree, with the "type"
property stripped. Use updateSaveInPresetComponents to restore from this object.
Dispatch/mechanics:
  content->exportAsValueTree() -> strip "type" from each child
    -> ValueTreeConverters::convertValueTreeToDynamicObject
Pair with:
  updateSaveInPresetComponents -- restore values from the returned object
  setUseCustomUserPresetModel -- typically used within custom save callbacks
Source:
  ScriptExpansion.cpp  createObjectForSaveInPresetComponents()
    -> content->exportAsValueTree()
    -> strips "type" property per child
    -> converts to DynamicObject
