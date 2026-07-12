ScriptSliderPack::getNumSliders() -> Integer

Thread safety: SAFE
Returns slider count from the currently resolved SliderPackData source.

Dispatch/mechanics:
  getUsedData(SliderPack) resolves source in order: referred holder -> connected processor slot -> owned object.

Pair with:
  set("sliderAmount", ...) -- changes slider count
  setWidthArray -- width map length depends on slider count
  referToData -- changing source changes reported count

Anti-patterns:
  - Do NOT assume non-zero count -- unresolved/invalid source can return 0.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::getNumSliders() -> ComplexDataScriptComponent::getUsedData(SliderPack)
