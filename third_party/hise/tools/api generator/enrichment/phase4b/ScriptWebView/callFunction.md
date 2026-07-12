ScriptWebView::callFunction(String javascriptFunction, NotUndefined args) -> undefined

Thread safety: UNSAFE
Calls a JavaScript function in the webview's global scope, passing the given
arguments. Dispatched asynchronously to the message thread -- the JS function
has NOT executed by the time the next HiseScript line runs.
Dispatch/mechanics:
  Captures WebViewData copy + args -> MessageManager::callAsync()
    -> WebViewData::call(functionName, args)
  With enablePersistence=true, the call is recorded and replayed when new
  webview instances are created.
Pair with:
  bindCallback -- for JS-to-HiseScript direction (return values from JS)
Anti-patterns:
  - Do NOT expect synchronous execution -- callFunction is fire-and-forget.
    Use bindCallback for return values from JavaScript.
  - The target function must exist in the webview's global window scope. If
    defined inside a module or closure, attach it to window explicitly in JS.
  - With enablePersistence=true, rapid repeated calls (e.g. from a timer)
    accumulate in the replay log. Timer-driven calls replay only the most
    recent value when the function name matches.
Source:
  ScriptingApiContent.cpp:5968  ScriptWebView::callFunction()
    -> MessageManager::callAsync([copy, func, args] { copy->call(func, args); })
