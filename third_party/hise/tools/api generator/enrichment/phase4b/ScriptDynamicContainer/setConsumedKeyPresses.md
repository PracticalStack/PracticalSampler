ScriptDynamicContainer::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Must be called before
setKeyPressCallback. Accepts "all" (exclusive), "all_nonexclusive", a key
description object, or an array of either.
Pair with:
  setKeyPressCallback -- must call setConsumedKeyPresses first
Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses()
