ScriptSliderPack::setWidthArray(Array normalizedWidths) -> undefined

Thread safety: UNSAFE
Sets non-uniform slider widths using normalized cumulative breakpoints.

Dispatch/mechanics:
  Stores widthArray on ScriptSliderPack and triggers change/update.
  SliderPackWrapper reapplies map; SliderPack uses it for both layout and mouse hit-testing.

Pair with:
  set("sliderAmount", ...) -- width map should have sliderAmount + 1 entries
  getNumSliders -- verify expected map size

Anti-patterns:
  - Do NOT ignore width-array length mismatch -- error is logged and rendering falls back to equal-width layout.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::setWidthArray() -> SliderPackWrapper::updateValue() -> SliderPack::setSliderWidths()
