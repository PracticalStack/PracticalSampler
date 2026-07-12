ScriptPanel::setLoadingCallback(Function loadingFunction) -> undefined

Thread safety: UNSAFE -- registers PreloadListener on SampleManager
Registers a callback that fires when sample preloading starts or finishes.
Pass a non-function value to remove the listener.
Callback signature: f(bool isPreloading)
Dispatch/mechanics:
  Registers panel as PreloadListener on SampleManager
  -> preloadStateChanged(isPreloading) calls loadRoutine.call1(isPreloading)
Source:
  ScriptingApiContent.cpp  ScriptPanel::setLoadingCallback()
