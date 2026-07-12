ScriptSlider::contains(Number valueToCheck) -> Integer

Thread safety: SAFE
Checks whether a value is inside the current range bounds for range sliders.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setStyle("Range");

Pair with:
  setStyle -- must be Range style
  setMinValue/setMaxValue -- defines the active range window

Anti-patterns:
  - Do NOT use in non-Range styles -- helper is range-specific and behavior is not meaningful.

Source:
  ScriptingApiContent.cpp:2054  range-style helper method group
