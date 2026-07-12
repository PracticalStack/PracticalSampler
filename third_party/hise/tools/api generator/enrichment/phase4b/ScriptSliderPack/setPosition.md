ScriptSliderPack::setPosition(Integer x, Integer y, Integer w, Integer h) -> undefined

Thread safety: UNSAFE
Sets component position and size in one call.

Required setup:
  const var spk = Content.addSliderPack("SP", 0, 0);

Pair with:
  getLocalBounds -- read reduced bounds from new geometry
  getWidth/getHeight -- scalar size reads

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack inherits ScriptComponent geometry write API
