ScriptedViewport::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback, replacing the default onControl handler. Pass false to revert to default. In List mode, value is row index. In Table mode with MultiColumnMode, value is [column, row] array.
Callback signature: f(ScriptComponent component, var value)
Required setup:
  const var vp = Content.getComponent("ViewportId");
  inline function onVpChanged(component, value) { /* ... */ };
  vp.setControlCallback(onVpChanged);
Pair with: changed (triggers this callback)
Anti-patterns: Must use `inline function` -- regular function references are rejected. Must have exactly 2 parameters. Passing false clears the custom callback.
Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback() -> customControlCallback = WeakCallbackHolder
