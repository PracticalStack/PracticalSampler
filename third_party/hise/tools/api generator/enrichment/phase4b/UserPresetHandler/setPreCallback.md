UserPresetHandler::setPreCallback(Function presetPreCallback) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Registers a callback that fires synchronously before a user preset is loaded.
Executes on the calling thread before voices are killed and loading thread
is entered. The callback argument depends on preprocessing mode:
  Without preprocessing: receives ScriptFile (preset passes through unchanged)
  With preprocessing: receives JSON object that can be modified in-place
Callback signature: f(var presetData)
Dispatch/mechanics:
  prePresetLoad() called from loadUserPresetFromValueTree
    -> Without preprocessing: creates ScriptFile, calls preCallback.callSync()
    -> With preprocessing: convertToJson(ValueTree) -> callSync(JSON)
       -> applyJSON(modified JSON) converts back to ValueTree
  Blocks the preset load pipeline until the callback returns
Pair with:
  setPostCallback -- asynchronous post-load counterpart
  setEnableUserPresetPreprocessing -- switch from File to JSON argument
  isOldVersion -- check preset version for migration logic
  isInternalPresetLoad -- distinguish DAW restore from user selection
Source:
  ScriptExpansion.cpp  setPreCallback()
    -> stores WeakCallbackHolder (1 arg)
  ScriptExpansion.cpp  prePresetLoad()
    -> preCallback.callSync() with ScriptFile or JSON
