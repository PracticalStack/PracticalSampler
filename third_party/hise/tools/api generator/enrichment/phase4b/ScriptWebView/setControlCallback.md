ScriptWebView::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback, replacing the default
onControl handler for this component.
Callback signature: f(ScriptComponent component, var value)
Anti-patterns:
  - The function MUST be declared with inline function -- regular function
    references are rejected with a script error
  - Must have exactly 2 parameters -- reports a script error if count is wrong
Pair with:
  changed -- triggers the registered control callback
Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback() (base class)
