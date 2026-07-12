ScriptSlider::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns [x, y, w, h] local bounds reduced by the inset amount on each side.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Source:
  ScriptingApiContent.cpp:2054  ScriptComponent local bounds helper
