ScriptDynamicContainer::changed() -> undefined

Thread safety: SAFE
Triggers the container's control callback (custom or default onControl) and notifies
value listeners. Fires the container-level callback, not dyncomp child value callbacks.
Anti-patterns:
  - Do NOT call during onInit -- logs a console message and returns without executing.
Pair with:
  setControlCallback -- set a custom handler for the container's own value
  getValue -- read the value that will be dispatched
Source:
  ScriptingApiContent.cpp  ScriptComponent::changed()
    -> checks onInit flag -> fires controlCallback or default onControl
