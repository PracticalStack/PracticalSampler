ScriptDynamicContainer::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback for key presses while this container has focus.
Callback signature: f(Object event)
Event properties: isFocusChange, character, specialKey, keyCode, description,
shift, cmd, alt. Focus events: isFocusChange=true, hasFocus.
Anti-patterns:
  - Do NOT call before setConsumedKeyPresses() -- reports a script error.
Pair with:
  setConsumedKeyPresses -- must be called first to define which keys to consume
Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback()
