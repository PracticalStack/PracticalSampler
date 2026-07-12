ScriptSlider::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Assigns a local scripted look-and-feel object to this slider and child script components.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Anti-patterns:
  - Do NOT assume only the parent changes -- local LAF applies recursively to child script components.

Source:
  ScriptingApiContent.cpp:2054  ScriptComponent local LAF assignment path
  ScriptComponentWrappers.cpp:1  SliderWrapper style and CSS realization
