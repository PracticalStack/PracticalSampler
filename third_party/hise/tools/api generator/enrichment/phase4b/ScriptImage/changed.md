ScriptImage::changed() -> undefined

Thread safety: SAFE
Triggers the control callback (custom via setControlCallback or default onControl).
Also notifies registered value listeners.
Dispatch/mechanics:
  Checks for custom controlFunction -> calls it with (component, value)
  Falls back to onControl callback if no custom function set
  If deferControlCallback is set, defers to message thread
Pair with:
  setValue -- sets the value that changed() reads and dispatches
  setControlCallback -- to assign a custom callback
Anti-patterns:
  - Do NOT call during onInit -- logs a console message and returns without executing
  - If the callback function throws an error, script execution after changed() is aborted
Source:
  ScriptingApiContent.cpp  ScriptComponent::changed()
    -> checks customControlCallback -> calls controlFunction(component, value)
    -> notifies valueListeners
