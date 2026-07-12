ScriptTable::setKeyPressCallback(Function keyboardFunction) -> undefined

Thread safety: UNSAFE
Registers a key and focus callback for consumed key presses.
Callback signature: keyboardFunction(Object event)

Pair with:
  setConsumedKeyPresses -- must configure consumed keys before registering callback

Anti-patterns:
  - Do NOT call before setConsumedKeyPresses -- runtime reports a script error and callback is not armed.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> keyboard callback registration
