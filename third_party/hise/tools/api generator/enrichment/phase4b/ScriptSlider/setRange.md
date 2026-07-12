ScriptSlider::setRange(Number min, Number max, Number stepSize) -> undefined

Thread safety: UNSAFE
Sets min, max, and step size in one call.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Dispatch/mechanics:
  wrapper updates HiSlider range, validates bounds, then applies skew and mode-specific adjustments
  invalid range state disables live slider interaction until corrected

Pair with:
  setMidPoint -- skew point must be valid for new range
  setValueNormalized/getValueNormalized -- conversions depend on configured range
  setMode -- mode defaults can rewrite range when previous defaults are untouched

Anti-patterns:
  - Do NOT use min >= max, negative step, or extreme invalid limits -- runtime slider widget is disabled.

Source:
  ScriptingApiContent.cpp:2054  setRange property mutation path
  ScriptComponentWrappers.cpp:1  SliderWrapper::updateSliderRange() validation and fallback behavior
