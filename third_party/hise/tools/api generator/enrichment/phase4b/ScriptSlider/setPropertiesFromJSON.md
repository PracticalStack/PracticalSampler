ScriptSlider::setPropertiesFromJSON(JSON jsonData) -> undefined

Thread safety: UNSAFE
Applies multiple slider properties from one JSON object.
Priority properties are applied first (mode for ScriptSlider), then remaining keys.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Dispatch/mechanics:
  property application is ordered by priorityProperties, then regular property list order
  each key routes through property setter dispatch for validation and side effects

Pair with:
  set/get -- single-property editing and verification

Source:
  ScriptingApiContent.cpp:2054  ValueTree property batch apply path with priorityProperties
