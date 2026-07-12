ScriptComboBox::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback, replacing the default
onControl handler for this component. Pass undefined to revert to onControl.
Callback signature: controlFunction(ScriptComponent component, var value)
Anti-patterns:
  - The function MUST be declared with inline function. Regular function references
    are rejected with a script error.
  - Must have exactly 2 parameters -- reports a script error if parameter count is wrong.
  - Do NOT use when the script processor has a DspNetwork forwarding controls to
    parameters -- reports an error.
Source:
  ScriptingApiContent.h  ScriptComponent::setControlCallback()
