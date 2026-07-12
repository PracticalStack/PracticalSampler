UserPresetHandler::setPostSaveCallback(Function presetPostSaveCallback) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Registers a callback that fires after a user preset has been saved. Executes
on the message thread. Receives a ScriptFile pointing to the saved file, or
undefined for non-file targets.
Callback signature: f(ScriptFile presetFile)
Pair with:
  setPostCallback -- for post-load events
  setPreCallback -- for pre-load events
Anti-patterns:
  - Do NOT call methods on presetFile without guarding -- it is undefined when
    the preset was saved to a non-file target. Guard with isDefined(presetFile).
Source:
  ScriptExpansion.cpp  setPostSaveCallback()
    -> stores WeakCallbackHolder (1 arg)
  ScriptExpansion.cpp  presetSaved()
    -> postSaveCallback fires on message thread
