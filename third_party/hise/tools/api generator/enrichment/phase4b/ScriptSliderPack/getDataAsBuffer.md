ScriptSliderPack::getDataAsBuffer() -> Buffer

Thread safety: SAFE
Returns a direct buffer reference to the bound SliderPackData values.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  setAllValues -- bulk value writes through callback-aware path
  setSliderAtIndex -- single index write with callback semantics

Anti-patterns:
  - Do NOT edit the returned buffer and expect setAllValues/setSliderAtIndex callback flow -- direct buffer edits bypass those helpers.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::getDataAsBuffer() -> active SliderPackData buffer handle
