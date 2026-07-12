ScriptPanel::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE -- modifies key press configuration
Defines which key presses this component consumes. Accepts "all" (exclusive),
"all_nonexclusive", or individual key descriptions as strings (JUCE format,
e.g. "ctrl + S", "F5") or JSON objects with keyCode, shift, cmd, alt, character.
Anti-patterns:
  - MUST be called BEFORE setKeyPressCallback -- reports a script error if called
    after or if an invalid key description is provided
Pair with:
  setKeyPressCallback -- register the callback after configuring consumed keys
Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses()
