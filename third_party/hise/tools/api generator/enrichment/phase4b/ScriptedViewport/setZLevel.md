ScriptedViewport::setZLevel(String zLevel) -> undefined

Thread safety: UNSAFE
Sets the depth level for this component among siblings. Valid values (case-sensitive): "Back", "Default", "Front", "AlwaysOnTop". Reports a script error for invalid values. Notifies z-level listeners on change.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setZLevel("AlwaysOnTop");
Source:
  ScriptingApiContent.cpp  ScriptComponent::setZLevel() -> validates string -> notifies zLevelListeners
