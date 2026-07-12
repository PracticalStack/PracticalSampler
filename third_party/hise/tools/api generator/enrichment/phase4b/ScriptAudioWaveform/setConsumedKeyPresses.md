ScriptAudioWaveform::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes. Must be called before
setKeyPressCallback. Accepts "all", "all_nonexclusive", JUCE key description
strings, or JSON objects with keyCode/modifiers.

Pair with:
  setKeyPressCallback -- register the handler after defining consumed keys

Source:
  ScriptingApiContent.cpp  ScriptComponent::setConsumedKeyPresses()
    -> parses key descriptions via KeyPress::createFromDescription()
