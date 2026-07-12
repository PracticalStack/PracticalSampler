ScriptSlider::getGlobalPositionY() -> Integer

Thread safety: SAFE
Returns absolute Y position by summing local offset with all parent offsets.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Source:
  ScriptingApiContent.cpp:2054  ScriptComponent coordinate aggregation helpers
