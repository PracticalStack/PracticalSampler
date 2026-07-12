ScriptedViewport::getWidth() -> Integer

Thread safety: SAFE
Returns the width property as an integer.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var w = vp.getWidth();
Source:
  ScriptingApiContent.cpp  ScriptComponent::getWidth() -> getScriptObjectProperty(Properties::width)
