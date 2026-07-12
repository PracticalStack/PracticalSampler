WavetableController::setErrorHandler(Function errorCallback) -> undefined

Thread safety: UNSAFE -- constructs a WeakCallbackHolder object with heap allocation
Sets a callback function that receives error messages during resynthesis.
Errors from FFT failures, Loris issues, etc. are routed through the
WeakErrorHandler interface to this callback.
Callback signature: f(String errorMessage)

Pair with:
  resynthesise -- errors from this process are routed to the handler
  setResynthesisOptions -- connects this controller as the error handler

Source:
  ScriptingApiObjects.cpp:5307  setErrorHandler()
    -> errorHandler = WeakCallbackHolder(f)
    -> handleErrorMessage() override calls errorHandler.call1(error)
