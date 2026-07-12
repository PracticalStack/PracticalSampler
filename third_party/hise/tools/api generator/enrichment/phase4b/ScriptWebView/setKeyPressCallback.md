ScriptWebView::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback that fires when a consumed key is pressed while this
component has focus. MUST call setConsumedKeyPresses() BEFORE this method.
Callback signature: f(Object event)
  Key event properties: isFocusChange, character, specialKey, isWhitespace,
  isLetter, isDigit, keyCode, description, shift, cmd, alt
  Focus event properties: isFocusChange, hasFocus
Anti-patterns:
  - MUST call setConsumedKeyPresses() first -- reports a script error if not
Pair with:
  setConsumedKeyPresses -- must be called before this method
Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback() (base class)
