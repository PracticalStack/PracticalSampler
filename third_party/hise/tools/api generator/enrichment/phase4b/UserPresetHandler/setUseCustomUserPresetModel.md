UserPresetHandler::setUseCustomUserPresetModel(Function loadCallback, Function saveCallback, Integer usePersistentObject) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Switches from the default user preset model (automatic saveInPreset component
serialization) to a custom model where script callbacks handle save and load.
The load callback receives a var with the custom data; the save callback receives
a preset name String and must return a JSON object. Must be called before
setCustomAutomation.
Callback signature: loadCallback(var data)
Callback signature: saveCallback(String presetName)
Dispatch/mechanics:
  Creates CustomStateManager (UserPresetStateManager with ID "CustomJSON")
  Load path: CustomStateManager::restoreFromValueTree -> loadCustomUserPreset listener
    -> customLoadCallback.callSync() with script lock held
  Save path: CustomStateManager::exportAsValueTree -> saveCustomUserPreset listener
    -> customSaveCallback.callSync() with script lock held
Anti-patterns:
  - [BUG] If either callback is not a valid function, the method silently returns
    without enabling the custom model. No error thrown. Subsequent
    setCustomAutomation calls fail with a confusing error message.
Pair with:
  setCustomAutomation -- requires custom data model as prerequisite
  createObjectForSaveInPresetComponents -- manually include component state
  updateSaveInPresetComponents -- restore component state in load callback
Source:
  ScriptExpansion.cpp  setUseCustomUserPresetModel()
    -> creates CustomStateManager
    -> registers with UserPresetHandler state manager list
