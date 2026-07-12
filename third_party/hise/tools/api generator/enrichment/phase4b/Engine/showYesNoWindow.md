Engine::showYesNoWindow(String title, String markdownMessage, Function callback) -> undefined

Thread safety: UNSAFE -- dispatched to message thread via MessageManager::callAsync
Shows yes/no dialog, calls callback with boolean result.
Callback signature: callback(bool ok)
Pair with:
  showMessageBox -- non-interactive message dialog
Source:
  ScriptingApi.cpp  Engine::showYesNoWindow()
    -> MessageManager::callAsync -> WeakCallbackHolder::call(result)
