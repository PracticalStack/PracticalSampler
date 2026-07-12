ScriptTable::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Reads the connected processor parameter and forwards it into setValue().

Pair with:
  setValue -- destination setter used by processor-connection refresh

Anti-patterns:
  - Do NOT assume it always updates -- without a valid processor/parameter connection this call silently does nothing.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> processor-connection value pull path
