ScriptSlider::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers keyboard and focus callback handling for this component.
Callback signature: f(Object event)

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setConsumedKeyPresses("all");

Pair with:
  setConsumedKeyPresses -- required key filter setup before callback registration

Anti-patterns:
  - Do NOT call before setConsumedKeyPresses -- registration reports an error.

Source:
  ScriptingApiContent.cpp:2054  keyboard callback registration path
