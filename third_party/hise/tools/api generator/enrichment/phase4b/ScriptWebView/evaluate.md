ScriptWebView::evaluate(String identifier, String jsCode) -> undefined

Thread safety: UNSAFE
Evaluates arbitrary JavaScript code in the webview. Dispatched asynchronously to
the message thread. The identifier is used for persistent call tracking -- when
enablePersistence is true, the code is stored and re-evaluated when new webview
instances are created. Reusing an identifier overwrites the previous stored code.
Dispatch/mechanics:
  Captures WebViewData copy + uid + code -> MessageManager::callAsync()
    -> WebViewData::evaluate(uid, jsCode)
    -> stores in initScripts[uid] when persistence enabled
Pair with:
  callFunction -- for calling named JS functions instead of raw code
  reset -- clears persistent call data including stored evaluate scripts
Anti-patterns:
  - Reusing an identifier overwrites the previous code stored under that key
    in the persistence system. Use unique identifiers per logical operation.
Source:
  ScriptingApiContent.cpp:5978  ScriptWebView::evaluate()
    -> MessageManager::callAsync([uid, copy, jsCode] { copy->evaluate(uid, jsCode); })
