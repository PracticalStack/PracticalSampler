ScriptSlider::getGlobalPositionX() -> Integer

Thread safety: SAFE
Returns absolute X position by summing local offset with all parent offsets.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Source:
  ScriptingApiContent.cpp:2054  ScriptComponent coordinate aggregation helpers
