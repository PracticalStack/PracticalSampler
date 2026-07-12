ScriptButton::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Accepts "all", "all_nonexclusive",
a JUCE key description string, a JSON key object, or an array of either.

Anti-patterns:
  - Must be called BEFORE setKeyPressCallback -- reports a script error if an
    invalid key description is provided

Pair with:
  setKeyPressCallback -- registers the callback that fires for consumed keys

Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses()
