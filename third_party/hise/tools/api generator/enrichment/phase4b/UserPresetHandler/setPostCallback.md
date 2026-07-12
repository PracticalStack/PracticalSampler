UserPresetHandler::setPostCallback(Function presetPostCallback) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Registers a callback that fires after a user preset has been loaded. Executes
asynchronously on the message thread after the entire background load completes
(macros, modules, MIDI automation, MPE all restored). Receives a ScriptFile
pointing to the loaded preset file, or undefined for non-file sources (DAW
state restore).
Callback signature: f(ScriptFile presetFile)
Dispatch/mechanics:
  loadUserPresetInternal completes on loading thread
    -> postPresetLoad() dispatches presetChanged to listeners
    -> callOnMessageThreadAfterSuspension -> postCallback fires on message thread
Pair with:
  setPreCallback -- synchronous pre-load counterpart
  setPostSaveCallback -- for post-save events
Anti-patterns:
  - Do NOT call methods on presetFile without guarding -- it is undefined when
    the preset was loaded from a DAW session restore. Guard with
    isDefined(presetFile).
Source:
  ScriptExpansion.cpp  setPostCallback()
    -> stores WeakCallbackHolder (1 arg)
  ScriptExpansion.cpp  presetChanged()
    -> postCallback.callOnMessageThreadAfterSuspension(ScriptFile)
