ScriptButton::setZLevel(String zLevel) -> undefined

Thread safety: UNSAFE
Sets the depth level for this component among its siblings. Valid values
(case-sensitive): "Back", "Default", "Front", "AlwaysOnTop". Reports a script
error for invalid values. Notifies z-level listeners on change.

Source:
  ScriptingApiContent.cpp  ScriptComponent::setZLevel()
