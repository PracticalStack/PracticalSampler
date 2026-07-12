ScriptSlider::setMidPoint(Colour valueForMidPoint) -> undefined

Thread safety: UNSAFE
Sets midpoint source for skew mapping in the current range.
Accepts numeric values, numeric strings, or the string sentinel "disabled".

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setRange(20.0, 20000.0, 1.0);
  sl.setMidPoint(1000.0);
  sl.setMidPoint("disabled");

Pair with:
  setRange -- midpoint must be valid for the active min/max bounds
  setValueNormalized/getValueNormalized -- midpoint affects conversion curve

Anti-patterns:
  - Do NOT rely on -1 as a universal disable token -- if range contains -1 it is treated as an active midpoint.
  - Use setMidPoint("disabled") for explicit no-skew behavior.

Source:
  ScriptingApiContent.cpp:2054  midpoint validation and range-skew update path
