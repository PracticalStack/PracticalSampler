MacroHandler::setUpdateCallback(Function callback) -> undefined

Thread safety: UNSAFE -- creates WeakCallbackHolder, increments reference count, and immediately fires the callback synchronously with the full macro data object (which allocates).
Registers a callback that fires whenever a macro connection changes. On registration, fires immediately with the current state (synchronous). Subsequent updates from UI or C++ fire asynchronously via JUCE AsyncUpdater (coalesced to message thread). Only one callback active at a time -- calling again replaces the previous one.
Callback signature: f(Array macroData)

Required setup:
  const var mh = Engine.createMacroHandler();

Dispatch/mechanics:
  Stores callback in WeakCallbackHolder (1 arg slot).
  Immediately calls sendUpdateMessage(sendNotificationSync) -> getMacroDataObject() ->
    callSync(&obj, 1) to deliver current state.
  Future changes arrive via macroConnectionChanged() -> triggerAsyncUpdate() ->
    handleAsyncUpdate() -> sendUpdateMessage(sendNotificationAsync) -> call1(obj).

Pair with:
  getMacroDataObject -- callback receives the same data format
  setMacroDataFromObject -- bulk changes fire a single coalesced callback on completion

Anti-patterns:
  - The callback fires immediately on registration with current state. Do NOT assume it
    only fires on future changes -- the callback body executes during onInit.
  - [BUG] No way to clear the callback. Passing false or a non-function value is silently
    ignored but does not unregister the previous callback. It remains active until the
    MacroHandler is garbage collected.
  - [BUG] Silently ignored if the argument is not a valid JavaScript function -- no error
    reported.

Source:
  ScriptingApiObjects.cpp  ScriptedMacroHandler::setUpdateCallback()
    -> stores in WeakCallbackHolder updateCallback
    -> sendUpdateMessage(sendNotificationSync) for immediate delivery
  ScriptingApiObjects.cpp  ScriptedMacroHandler::macroConnectionChanged()
    -> triggerAsyncUpdate() -> handleAsyncUpdate() -> sendUpdateMessage(async)
