ScriptAudioWaveform::setZLevel(String zLevel) -> undefined

Thread safety: UNSAFE
Sets the depth level for this component among its siblings.
Valid values: "Back", "Default", "Front", "AlwaysOnTop" (case-sensitive).
Reports a script error for invalid values.

Source:
  ScriptingApiContent.cpp  ScriptComponent::setZLevel()
    -> validates string -> notifies ZLevelListeners
