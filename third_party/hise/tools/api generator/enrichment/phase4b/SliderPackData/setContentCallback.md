SliderPackData::setContentCallback(var contentFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder for the callback function.
Registers a callback that fires when slider values change. The callback receives a
single integer: the index of the changed slider, or -1 for bulk operations
(e.g., setAllValues(), fromBase64()).
Callback signature: f(int sliderIndex)
Dispatch/mechanics:
  setCallbackInternal(isDisplay=false, f) -> creates WeakCallbackHolder(1 arg)
  -> onComplexDataEvent(ContentChange) -> contentCallback.call1(sliderIndex)
Pair with:
  setDisplayCallback -- separate callback for ruler/position changes
  setValue -- triggers content callback with the changed index
  setAllValues -- triggers content callback with index -1
Source:
  ScriptingApiObjects.cpp  ScriptComplexDataReferenceBase::setCallbackInternal()
    -> validates isJavascriptFunction
    -> stores WeakCallbackHolder with 1 argument
