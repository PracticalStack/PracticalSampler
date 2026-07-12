MacroHandler::setExclusiveMode(Integer shouldBeExclusive) -> undefined

Thread safety: SAFE
Enables or disables exclusive mode for macro connections. When enabled, each macro slot can only connect to a single target parameter -- adding a new connection removes the previous one. Setting is shared with the global MacroManager and persists for the session.

Required setup:
  const var mh = Engine.createMacroHandler();

Dispatch/mechanics:
  Delegates directly to MacroManager::setExclusiveMode(bool) on MainController.
  Sets a boolean flag -- no allocation, no notification dispatch.

Pair with:
  setMacroDataFromObject -- exclusive mode affects how connections are added during rebuild

Source:
  ScriptingApiObjects.cpp  ScriptedMacroHandler::setExclusiveMode()
    -> MainController::MacroManager::setExclusiveMode(shouldBeExclusive)
