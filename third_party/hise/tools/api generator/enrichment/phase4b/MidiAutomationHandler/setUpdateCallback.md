MidiAutomationHandler::setUpdateCallback(Function callback) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder and invokes the callback synchronously once during registration.
Registers a callback that fires whenever the MIDI automation configuration changes.
The callback receives the complete current automation data array (same format as
getAutomationDataObject()). Fires on: MIDI learn completion, entry removal, preset
restore, clear. Called immediately once during registration with current state.
Callback signature: f(Array automationData)

Required setup:
  const var mah = Engine.createMidiAutomationHandler();

Dispatch/mechanics:
  Creates WeakCallbackHolder with 1 arg slot, tagged "onMidiAutomationUpdate"
    -> callSync() immediately with getAutomationDataObject() result
  At runtime: MidiControllerAutomationHandler (SafeChangeBroadcaster)
    -> sendChangeMessage() [async] or sendSynchronousChangeMessage() [preset load]
    -> changeListenerCallback() -> updateCallback.call1(getAutomationDataObject())

Pair with:
  getAutomationDataObject -- same data format the callback receives
  setAutomationDataFromObject -- programmatic changes that trigger the callback

Anti-patterns:
  - Do NOT pass a non-function value (false, undefined, number) to "clear" the
    callback -- silently ignored, previous callback stays active. There is no
    unregister mechanism.
  - Do NOT call setAutomationDataFromObject() from inside this callback --
    causes infinite recursion during synchronous preset loads.
  - Do NOT depend on state set up after the setUpdateCallback() call in the
    callback body -- the callback fires immediately during registration.

Source:
  ScriptingApiObjects.cpp:10074  ScriptedMidiAutomationHandler::setUpdateCallback()
    -> WeakCallbackHolder(getScriptProcessor(), this, callback, 1)
    -> updateCallback.incRefCount()
    -> updateCallback.addAsSource(this, "onMidiAutomationUpdate")
    -> callSync(&obj, 1) for immediate initial invocation
  ScriptingApiObjects.cpp         changeListenerCallback()
    -> updateCallback.call1(getAutomationDataObject())
