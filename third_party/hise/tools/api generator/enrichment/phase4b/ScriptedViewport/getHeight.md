ScriptedViewport::getHeight() -> Integer

Thread safety: SAFE
Returns the height property as an integer.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var h = vp.getHeight();
Source:
  ScriptingApiContent.cpp  ScriptComponent::getHeight() -> getScriptObjectProperty(Properties::height)
