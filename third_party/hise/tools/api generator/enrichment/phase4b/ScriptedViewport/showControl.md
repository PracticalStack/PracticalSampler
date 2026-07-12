ScriptedViewport::showControl(Integer shouldBeVisible) -> undefined

Thread safety: UNSAFE
Sets the visible property with change message notification. 1 = show, 0 = hide.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.showControl(1);
Pair with: fadeComponent (animated visibility toggle)
Source:
  ScriptingApiContent.cpp  ScriptComponent::showControl() -> setScriptObjectProperty(visible) -> sendChangeMessage()
