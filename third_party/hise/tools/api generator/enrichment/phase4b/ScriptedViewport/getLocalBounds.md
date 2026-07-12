ScriptedViewport::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns [x, y, w, h] representing the local bounds reduced by the given pixel amount from each edge. Starts at [0, 0, width, height].
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var bounds = vp.getLocalBounds(0);
Source:
  ScriptingApiContent.cpp  ScriptComponent::getLocalBounds() -> Rectangle<int>::reduced()
