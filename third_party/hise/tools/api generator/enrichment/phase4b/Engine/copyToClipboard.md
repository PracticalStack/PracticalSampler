Engine::copyToClipboard(String textToCopy) -> undefined

Thread safety: UNSAFE -- OS clipboard API calls (message thread interaction)
Copies the given text to the system clipboard.
Pair with:
  getClipboardContent -- read text back from clipboard
Source:
  ScriptingApi.cpp  Engine::copyToClipboard()
    -> SystemClipboard::copyTextToClipboard(textToCopy)
