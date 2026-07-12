ScriptSlider::setValuePopupFunction(Function newFunction) -> undefined

Thread safety: UNSAFE
Sets callback that formats popup text shown while dragging when popup display is enabled.
Callback signature: f(double value)

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.set("showValuePopup", "Above");

Dispatch/mechanics:
  wrapper calls callback through NativeFunctionArgs with one numeric argument during popup rendering

Pair with:
  set("showValuePopup", ...) -- popup formatter only matters when popup rendering is enabled

Anti-patterns:
  - Do NOT use wrong callback arity -- function must accept exactly 1 argument.

Source:
  ScriptingApiContent.cpp:2054  popup callback registration
  ScriptComponentWrappers.cpp:1  SliderWrapper::getTextForValuePopup() callback invocation
