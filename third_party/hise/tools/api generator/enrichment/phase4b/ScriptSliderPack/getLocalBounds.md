ScriptSliderPack::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns local bounds reduced by the given inset amount.

Required setup:
  const var spk = Content.addSliderPack("SP", 10, 10);

Pair with:
  setPosition -- updates bounds returned by this method
  getWidth/getHeight -- scalar width and height queries

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits ScriptComponent bounds API
