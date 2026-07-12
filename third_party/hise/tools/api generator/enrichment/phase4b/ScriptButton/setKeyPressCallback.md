ScriptButton::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback for key press events while this component has focus. Must call
setConsumedKeyPresses() BEFORE this method. Reports a script error otherwise.
Callback signature: f(Object event)

The event object has two shapes:
  Key press: { isFocusChange: false, character, specialKey, isWhitespace, isLetter,
    isDigit, keyCode, description, shift, cmd, alt }
  Focus change: { isFocusChange: true, hasFocus }

Pair with:
  setConsumedKeyPresses -- must be called first to define which keys to capture

Anti-patterns:
  - Do NOT call without calling setConsumedKeyPresses first -- script error

Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback()
