ScriptPanel::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE -- registers key press listener
Registers a callback for key press events while this component has focus.
MUST call setConsumedKeyPresses() BEFORE this method.
Callback signature: f(Object event)
Key press event properties: isFocusChange (false), character, specialKey, isWhitespace,
isLetter, isDigit, keyCode, description, shift, cmd, alt.
Focus change event properties: isFocusChange (true), hasFocus.
Anti-patterns:
  - MUST call setConsumedKeyPresses() BEFORE this method -- reports script error otherwise
Pair with:
  setConsumedKeyPresses -- configure which keys to intercept (required first)
Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback()
