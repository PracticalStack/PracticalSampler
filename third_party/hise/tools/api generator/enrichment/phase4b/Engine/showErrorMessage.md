Engine::showErrorMessage(String message, bool isCritical) -> undefined

Thread safety: UNSAFE -- string construction, UI overlay state management
Shows error overlay. isCritical=true disables Ignore button. isCritical=false includes it.
Pair with:
  showMessage -- informational overlay
  showMessageBox -- icon-based message dialog
Source:
  ScriptingApi.cpp  Engine::showErrorMessage()
    -> MainController::sendOverlayMessage(CriticalCustomErrorMessage or CustomErrorMessage)
