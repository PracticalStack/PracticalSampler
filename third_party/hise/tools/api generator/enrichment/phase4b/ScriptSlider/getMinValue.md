ScriptSlider::getMinValue() -> Double

Thread safety: SAFE
Returns the lower handle value for range sliders.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setStyle("Range");

Pair with:
  setMinValue/setMaxValue -- updates and reads range handles
  contains -- checks membership inside current range window

Anti-patterns:
  - Do NOT call outside Range style -- returns fallback 0.0 and logs an error.

Source:
  ScriptingApiContent.cpp:2054  range-style helper method group
