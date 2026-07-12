ScriptSlider::getMaxValue() -> Double

Thread safety: SAFE
Returns the upper handle value for range sliders.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setStyle("Range");

Pair with:
  setMaxValue/setMinValue -- updates and reads range handles
  contains -- checks membership inside current range window

Anti-patterns:
  - Do NOT call outside Range style -- returns fallback 1.0 and logs an error.

Source:
  ScriptingApiContent.cpp:2054  range-style helper method group
