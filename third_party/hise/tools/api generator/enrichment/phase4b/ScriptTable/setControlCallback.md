ScriptTable::setControlCallback(Function controlFunction) -> undefined

Thread safety: UNSAFE
Sets a custom inline control callback for this component.
Callback signature: controlFunction(ScriptComponent component, var value)

Pair with:
  changed -- manually trigger control callback

Anti-patterns:
  - Do NOT use non-inline callback definitions -- callback must be inline.
  - Do NOT use callback signatures other than 2 parameters -- invalid signatures are rejected.
  - Do NOT expect custom callbacks when processor-parameter forwarding is active -- custom callback registration is rejected.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> callback validator and registration
