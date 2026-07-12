ScriptSliderPack::getSliderValueAt(Integer index) -> Double

Thread safety: UNSAFE
Returns value at one slider index and updates displayed-index highlight state.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  setSliderAtIndex -- writes the same indexed value path
  setAllValues -- bulk alternative

Anti-patterns:
  - Do NOT rely on exceptions for invalid indices -- out-of-range reads return default value.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::getSliderValueAt() -> SliderPackData indexed read
