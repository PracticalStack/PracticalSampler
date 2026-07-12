Engine::showMessageBox(String title, String markdownMessage, int type) -> undefined

Thread safety: UNSAFE -- dispatched to message thread via MessageManager::callAsync
Shows modal message box with title, markdown body, and OK button.
Type: 0=Info, 1=Warning, 2=Question, 3=Error (icon type).
Pair with:
  showYesNoWindow -- interactive yes/no dialog
Source:
  ScriptingApi.cpp  Engine::showMessageBox()
    -> MessageManager::callAsync -> PresetHandler::showMessageWindow(type)
