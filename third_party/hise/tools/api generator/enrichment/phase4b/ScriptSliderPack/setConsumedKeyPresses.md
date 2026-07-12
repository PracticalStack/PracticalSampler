ScriptSliderPack::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes before key callback registration.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  setKeyPressCallback -- callback only receives events for consumed keys/focus updates

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits ScriptComponent keyboard-consumption API
