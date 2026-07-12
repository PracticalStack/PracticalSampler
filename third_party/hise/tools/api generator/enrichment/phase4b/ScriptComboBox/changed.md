ScriptComboBox::changed() -> undefined

Thread safety: SAFE
Triggers the control callback (custom via setControlCallback or default onControl).
Also notifies registered value listeners.
Pair with:
  setControlCallback -- set a custom callback to be triggered
  setValue -- set value before calling changed() to propagate updates
Anti-patterns:
  - Do NOT call during onInit -- logs a console message and returns without executing.
  - If the callback function throws an error, script execution after changed() is aborted.
Source:
  ScriptingApiContent.h  ScriptComponent::changed()
