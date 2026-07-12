ScriptComboBox::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Must be called before
setKeyPressCallback. Accepts "all", "all_nonexclusive", a string key
description, an object, or an array of either.
Pair with:
  setKeyPressCallback -- register callback after defining consumed keys
Anti-patterns:
  - Must be called BEFORE setKeyPressCallback -- reports a script error if an
    invalid key description is provided.
Source:
  ScriptingApiContent.h  ScriptComponent::setConsumedKeyPresses()
