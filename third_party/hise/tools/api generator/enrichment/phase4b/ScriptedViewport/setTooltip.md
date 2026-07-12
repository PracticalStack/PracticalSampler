ScriptedViewport::setTooltip(String tooltip) -> undefined

Thread safety: UNSAFE
Sets the tooltip text to display on mouse hover.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setTooltip("Hover text");
Source:
  ScriptingApiContent.cpp  ScriptComponent::setTooltip() -> setScriptObjectProperty(Properties::tooltip, ...)
