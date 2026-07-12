ScriptButton::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback, replacing the default
onControl handler for this component. Pass undefined to revert to default.
Callback signature: f(ScriptComponent component, var value)

Anti-patterns:
  - Must use inline function -- regular function references are rejected with a script error
  - Must have exactly 2 parameters -- reports a script error if count is wrong
  - Do NOT use with a DspNetwork that forwards controls to parameters -- reports an error

Pair with:
  changed -- manually triggers the registered callback

Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback()
