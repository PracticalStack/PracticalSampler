ScriptedViewport::getGlobalPositionX() -> Integer

Thread safety: SAFE
Returns the absolute x-position relative to the interface root, computed by recursively adding parent component x-offsets.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var x = vp.getGlobalPositionX();
Pair with: getGlobalPositionY (absolute y-position)
Source:
  ScriptingApiContent.cpp  ScriptComponent::getGlobalPositionX() -> recursive parent x accumulation
