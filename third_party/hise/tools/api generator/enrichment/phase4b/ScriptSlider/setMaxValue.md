ScriptSlider::setMaxValue(Number max) -> undefined

Thread safety: UNSAFE
Sets the upper handle value for range sliders and schedules async UI update.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setStyle("Range");

Pair with:
  setMinValue/getMaxValue -- range-handle editing and verification

Anti-patterns:
  - Do NOT call outside Range style -- call logs and does not apply as intended.

Source:
  ScriptingApiContent.cpp:2054  range-style helper method group
