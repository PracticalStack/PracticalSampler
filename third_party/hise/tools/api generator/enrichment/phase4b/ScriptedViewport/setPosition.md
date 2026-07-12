ScriptedViewport::setPosition(Integer x, Integer y, Integer w, Integer h) -> undefined

Thread safety: UNSAFE
Sets the component's position and size in one call. Directly sets x, y, width, height properties on the property tree.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setPosition(10, 10, 400, 300);
Source:
  ScriptingApiContent.cpp  ScriptComponent::setPosition() -> setScriptObjectProperty(x, y, width, height)
