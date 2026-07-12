SliderPackData::setDisplayCallback(var displayFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder for the callback function.
Registers a callback that fires when the display/ruler position changes. The callback
receives a single float: the display index position. Typically driven by a
ScriptSliderPack UI component's playback indicator.
Callback signature: f(float displayIndex)
Dispatch/mechanics:
  setCallbackInternal(isDisplay=true, f) -> creates WeakCallbackHolder(1 arg)
  -> onComplexDataEvent(DisplayIndex) -> displayCallback.call1(displayIndex)
Pair with:
  getCurrentlyDisplayedIndex -- poll the display index instead of using a callback
  setContentCallback -- separate callback for value changes
Source:
  ScriptingApiObjects.cpp  ScriptComplexDataReferenceBase::setCallbackInternal()
    -> validates isJavascriptFunction
    -> stores WeakCallbackHolder with 1 argument
