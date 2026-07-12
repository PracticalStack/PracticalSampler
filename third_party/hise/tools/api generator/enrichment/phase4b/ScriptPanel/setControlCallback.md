ScriptPanel::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE -- modifies callback registration
Assigns a custom inline function as the control callback, replacing the default
onControl handler. Pass undefined to revert to default onControl.
Callback signature: f(ScriptComponent component, var value)
Anti-patterns:
  - MUST use inline function -- regular function references are rejected with script error
  - Must have exactly 2 parameters -- script error if parameter count is wrong
  - Do NOT use when the script processor has a DspNetwork forwarding controls to parameters
Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback()
