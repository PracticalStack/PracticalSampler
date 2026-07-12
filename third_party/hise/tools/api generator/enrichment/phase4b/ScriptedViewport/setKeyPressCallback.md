ScriptedViewport::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a callback that fires when a consumed key is pressed while this component has focus. Must call setConsumedKeyPresses() BEFORE this method.
Callback signature: f(Object event)
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setConsumedKeyPresses("all");
  inline function onKey(event) { /* ... */ };
  vp.setKeyPressCallback(onKey);
Dispatch/mechanics: Stores callback as WeakCallbackHolder.

Example key event:
  { "isFocusChange": false, "character": "a", "specialKey": false,
    "isWhitespace": false, "isLetter": true, "isDigit": false,
    "keyCode": 65, "description": "a",
    "shift": false, "cmd": false, "alt": false }

Example focus event:
  { "isFocusChange": true, "hasFocus": true }
Pair with: setConsumedKeyPresses (must be called first to define which keys are consumed)
Anti-patterns: Reports a script error if setConsumedKeyPresses has not been called first.
Source:
  ScriptingApiContent.cpp  ScriptComponent::setKeyPressCallback() -> keyPressCallback = WeakCallbackHolder
