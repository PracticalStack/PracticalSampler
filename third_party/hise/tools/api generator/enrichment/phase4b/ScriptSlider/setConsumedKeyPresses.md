ScriptSlider::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes for keyboard callback handling.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setConsumedKeyPresses("all");

Pair with:
  setKeyPressCallback -- register callback after key-consumption mapping is configured

Anti-patterns:
  - Do NOT pass invalid key descriptors -- invalid entries are rejected and not registered.

Source:
  ScriptingApiContent.cpp:2054  keyboard consumption map setup
