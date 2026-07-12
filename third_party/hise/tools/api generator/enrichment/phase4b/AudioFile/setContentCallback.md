AudioFile::setContentCallback(Function contentFunction) -> undefined

Thread safety: UNSAFE -- creates WeakCallbackHolder (heap allocation).
Registers a callback that fires when audio content changes (file loaded, buffer
modified, range changed). The callback takes no arguments; 'this' inside the
callback points to the AudioFile that changed.
Callback signature: contentFunction()

Required setup:
  const var af = Engine.createAndRegisterAudioFile(0);

Dispatch/mechanics:
  setCallbackInternal(false, f)
    -> creates WeakCallbackHolder(processor, this, f, 1)
    -> sets this as 'this' object for callback
    -> onComplexDataEvent routes non-DisplayIndex events to contentCallback

Pair with:
  setDisplayCallback -- for playback position updates (separate event type)
  update -- manually trigger content change notification

Anti-patterns:
  - Do NOT register individual callbacks on 12+ AudioFile handles -- use
    Broadcaster.attachToComplexData("AudioFile.Content", ...) for multi-slot monitoring.

Source:
  ScriptingApiObjects.cpp:1547  ScriptComplexDataReferenceBase::setCallbackInternal()
    -> WeakCallbackHolder with setThisObject(this)
    -> onComplexDataEvent: non-DisplayIndex -> contentCallback.call1(data)
