UserPresetHandler::setParameterGestureCallback(Function callbackFunction) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Registers a callback that fires on DAW host parameter gesture begin/end events.
Called synchronously via callSync when the host touches/releases a parameter.
Callback signature: f(int automationType, int slotIndex, bool startGesture)
  automationType: 0=Macro, 1=CustomAutomation, 2=ScriptControl, 3=NKSWrapper
Dispatch/mechanics:
  AudioProcessorListener::audioProcessorParameterChangeGestureBegin/End
    -> onParameterGesture(startGesture, parameterIndex)
    -> parameterGestureCallback.callSync(NativeFunctionArgs with 3 args)
Anti-patterns:
  - [BUG] WeakCallbackHolder is initialized with numExpectedArgs=2, and the
    parse-time diagnostic reports 2 expected args. But the actual call passes
    3 args (type, slotIndex, startGesture). Write a 3-parameter callback to
    receive all data.
Source:
  ScriptExpansion.cpp  setParameterGestureCallback()
    -> stores WeakCallbackHolder
  ScriptExpansion.cpp  onParameterGesture()
    -> var args[3] = {type, slotIndex, startGesture}
    -> parameterGestureCallback.callSync(NativeFunctionArgs)
