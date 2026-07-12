ScriptedViewport::changed() -> undefined

Thread safety: SAFE
Triggers the control callback (custom via setControlCallback or default onControl). Also notifies registered value listeners.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.changed();
Dispatch/mechanics: Checks if called during onInit (aborts with console message if so). If deferControlCallback is set, defers to message thread. Invokes the assigned callback function with (component, value). Notifies value listeners.
Pair with: setControlCallback (assigns the callback this triggers), getValue (retrieves the value passed to callback)
Anti-patterns: Do not call during onInit -- silently returns without executing. If the callback throws, script execution after changed() is aborted.
Source:
  ScriptingApiContent.cpp  ScriptComponent::changed() -> customControlCallback.call() / onControl
