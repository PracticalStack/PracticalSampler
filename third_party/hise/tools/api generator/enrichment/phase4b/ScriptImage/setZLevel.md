ScriptImage::setZLevel(String zLevel) -> undefined

Thread safety: UNSAFE
Sets the depth level for this component among its siblings. Reports a script
error if the value is not one of: "Back", "Default", "Front", "AlwaysOnTop"
(case-sensitive). Notifies z-level listeners when changed.
Source:
  ScriptingApiContent.cpp  ScriptComponent::setZLevel()
    -> validates against 4 valid strings -> notifies ZLevelListeners
