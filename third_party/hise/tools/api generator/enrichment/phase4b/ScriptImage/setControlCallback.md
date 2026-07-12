ScriptImage::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Assigns a custom inline function as the control callback, replacing the default
onControl handler. Pass undefined to revert to default onControl.
Callback signature: f(ScriptComponent component, var value)
Required setup:
  const var img = Content.addImage("MyImage", 0, 0);
  img.set("allowCallbacks", "Clicks Only");
Pair with:
  changed -- triggers this callback with the current value
Anti-patterns:
  - Must use inline function -- regular function references are rejected with script error
  - Must have exactly 2 parameters -- wrong count triggers script error
  - If processorId/parameterId are configured, custom callback is bypassed
Source:
  ScriptingApiContent.cpp  ScriptComponent::setControlCallback()
    -> stores controlFunction -> used by changed() dispatch
