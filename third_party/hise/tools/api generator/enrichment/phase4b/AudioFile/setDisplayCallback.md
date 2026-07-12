AudioFile::setDisplayCallback(Function displayFunction) -> undefined

Thread safety: UNSAFE -- creates WeakCallbackHolder (heap allocation).
Registers a callback that fires when the display index changes (e.g. during
audio playback). The callback receives the current display position as a
floating-point value.
Callback signature: displayFunction(double displayIndex)

Required setup:
  const var af = Engine.createAndRegisterAudioFile(0);

Dispatch/mechanics:
  setCallbackInternal(true, f)
    -> creates WeakCallbackHolder(processor, this, f, 1)
    -> onComplexDataEvent routes DisplayIndex events to displayCallback

Pair with:
  getCurrentlyDisplayedIndex -- poll the position without a callback
  setContentCallback -- for content change events (separate event type)

Source:
  ScriptingApiObjects.cpp:1547  ScriptComplexDataReferenceBase::setCallbackInternal()
    -> WeakCallbackHolder with setThisObject(this)
    -> onComplexDataEvent: DisplayIndex -> displayCallback.call1(data)
