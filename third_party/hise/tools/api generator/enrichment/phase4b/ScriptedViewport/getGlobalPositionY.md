ScriptedViewport::getGlobalPositionY() -> Integer

Thread safety: SAFE
Returns the absolute y-position relative to the interface root, computed by recursively adding parent component y-offsets.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var y = vp.getGlobalPositionY();
Pair with: getGlobalPositionX (absolute x-position)
Source:
  ScriptingApiContent.cpp  ScriptComponent::getGlobalPositionY() -> recursive parent y accumulation
