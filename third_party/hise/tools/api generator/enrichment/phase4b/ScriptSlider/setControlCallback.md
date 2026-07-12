ScriptSlider::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Sets a custom control callback for slider value changes.
Callback signature: f(ScriptComponent component, var value)

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Pair with:
  changed -- executes this callback for scripted change events

Anti-patterns:
  - Do NOT pass a non-inline function -- callback must be inline.
  - Do NOT use wrong arity -- callback must declare exactly 2 parameters.
  - Do NOT rely on this in forwarded scriptnode parameter mode -- registration can report an error.

Source:
  ScriptingApiContent.cpp:2054  setControlCallback registration and callback validator
