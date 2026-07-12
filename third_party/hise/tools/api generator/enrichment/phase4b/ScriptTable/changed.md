ScriptTable::changed() -> undefined

Thread safety: SAFE
Triggers this component's control callback (custom setControlCallback or default onControl).

Pair with:
  setControlCallback -- define the callback invoked by changed()
  getValue -- read current value inside callback logic

Anti-patterns:
  - Do NOT call during onInit -- call is ignored.
  - Do NOT expect immediate execution when deferControlCallback is enabled -- callback is deferred to the message thread.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> Wrapper::changed()
