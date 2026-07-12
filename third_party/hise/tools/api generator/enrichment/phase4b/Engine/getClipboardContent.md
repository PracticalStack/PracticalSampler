Engine::getClipboardContent() -> String

Thread safety: UNSAFE -- OS clipboard API calls (message thread interaction)
Returns the current text content of the system clipboard.
Pair with:
  copyToClipboard -- write text to clipboard
Source:
  ScriptingApi.cpp  Engine::getClipboardContent()
    -> SystemClipboard::getTextFromClipboard()
