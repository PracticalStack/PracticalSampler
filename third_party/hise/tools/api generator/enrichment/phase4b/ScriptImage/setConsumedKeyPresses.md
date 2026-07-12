ScriptImage::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Accepts "all" (exclusive),
"all_nonexclusive", individual key description strings (JUCE format like
"ctrl + S", "F5"), or JSON objects with keyCode/shift/cmd/alt fields.
Pair with:
  setKeyPressCallback -- must be called AFTER setConsumedKeyPresses
Anti-patterns:
  - Must be called BEFORE setKeyPressCallback -- reversed order triggers a script error
Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses()
    -> parses key descriptions via KeyPress::createFromDescription()
    -> stores consumed key list for callback filtering
