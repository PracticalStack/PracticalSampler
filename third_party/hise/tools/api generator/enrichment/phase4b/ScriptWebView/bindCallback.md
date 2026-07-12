ScriptWebView::bindCallback(String callbackId, Function functionToCall) -> undefined

Thread safety: UNSAFE
Binds a HiseScript function to a JavaScript callback identifier. The JS side
invokes it using the Promise pattern: callbackId(args).then(result => { ... }).
The HiseScript function receives a single argument and can return a value that
resolves the JS Promise.
Callback signature: f(var args)
Dispatch/mechanics:
  Creates HiseScriptCallback wrapping a WeakCallbackHolder(processor, 1 arg)
    -> setHighPriority(), setThisObject(webview)
    -> WebViewData::addCallback(callbackId, callback)
  On JS invocation: WeakCallbackHolder::callSync() -- synchronous on the
  choc::WebView callback thread
Pair with:
  callFunction -- HiseScript-to-JS direction (complement of bindCallback)
  evaluate -- alternative for pushing JS code rather than calling named functions
Anti-patterns:
  - Avoid long-running operations inside the callback -- it executes
    synchronously on the webview's callback thread. Defer heavy work to a timer.
  - The JS side must use the Promise pattern: callbackId(args).then(...)
    Calling without .then() still works but discards the return value.
Source:
  ScriptingApiContent.cpp:5963  ScriptWebView::bindCallback()
    -> new HiseScriptCallback(this, callbackId, functionToCall)
    -> data->addCallback(callbackId, callback)
