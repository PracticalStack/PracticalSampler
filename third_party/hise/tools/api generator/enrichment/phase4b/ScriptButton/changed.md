ScriptButton::changed() -> undefined

Thread safety: SAFE
Triggers the control callback (custom via setControlCallback or default onControl).
Also notifies registered value listeners.

Pair with:
  setControlCallback -- to set a custom callback that changed() will invoke
  setValue -- set the value before calling changed() to trigger the callback

Anti-patterns:
  - Do NOT call during onInit -- logs a console message and returns without executing
  - If the callback function throws an error, further script execution after changed() is aborted

Source:
  ScriptingApiContent.cpp  ScriptComponent::changed()
    -> customControlCallback or default onControl callback
    -> notifies value listeners
