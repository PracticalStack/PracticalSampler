ScriptSlider::setStyle(String style) -> undefined

Thread safety: UNSAFE
Sets slider style (Knob, Horizontal, Vertical, Range).

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Dispatch/mechanics:
  style string maps to JUCE slider style and wrapper updates drag/textbox/CSS behavior
  Range maps to TwoValueHorizontal and enables range-only helper methods

Pair with:
  setMinValue/setMaxValue/getMinValue/getMaxValue/contains -- valid only in Range style
  setMode -- style and mode jointly shape display and interaction behavior

Anti-patterns:
  - Do NOT pass invalid style strings -- property text and runtime style can diverge.

Source:
  ScriptingApiContent.cpp:2054  setStyle string mapping and property update
  ScriptComponentWrappers.cpp:1  SliderWrapper::updateSliderStyle() runtime style realization
