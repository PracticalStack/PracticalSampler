ScriptDynamicContainer::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback for the container's own
value, replacing the default onControl handler. Pass undefined to revert to default.
This handles container-level value changes, not dyncomp child values.
Callback signature: f(ScriptComponent component, var value)
Anti-patterns:
  - Do NOT use a regular function -- must be declared with inline function. Script error otherwise.
  - Must have exactly 2 parameters -- script error if parameter count is wrong.
Pair with:
  changed -- trigger this callback manually
  setValueCallback -- separate system for dyncomp child value changes
Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback()
