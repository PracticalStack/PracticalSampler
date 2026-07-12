ScriptComboBox::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback that fires when a consumed key is pressed while this
component has focus. MUST call setConsumedKeyPresses() BEFORE this method.
Callback signature: keyboardFunction(Object event)
Pair with:
  setConsumedKeyPresses -- must define consumed keys before registering callback
Anti-patterns:
  - Do NOT call without calling setConsumedKeyPresses first -- reports a script error.
Source:
  ScriptingApiContent.h  ScriptComponent::setKeyPressCallback()
