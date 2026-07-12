Engine::showMessage(String message) -> undefined

Thread safety: UNSAFE -- string construction, UI overlay state management
Shows informational overlay with OK button (CustomInformation state).
Pair with:
  showErrorMessage -- error overlay
  showYesNoWindow -- interactive dialog
Source:
  ScriptingApi.cpp  Engine::showMessage()
    -> MainController::sendOverlayMessage(CustomInformation, message)
