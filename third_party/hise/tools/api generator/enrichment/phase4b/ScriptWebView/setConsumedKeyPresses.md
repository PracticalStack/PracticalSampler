ScriptWebView::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Must be called BEFORE
setKeyPressCallback. Accepts "all" (exclusive), "all_nonexclusive", a key
description object, or an array of either.
Anti-patterns:
  - Must be called BEFORE setKeyPressCallback -- reports a script error if
    an invalid key description is provided
Pair with:
  setKeyPressCallback -- must call setConsumedKeyPresses first
Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses() (base class)
