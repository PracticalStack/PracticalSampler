ScriptTable::get(String propertyName) -> var

Thread safety: SAFE
Returns the current value of a ScriptTable property. See set() for the full
property list.

Pair with:
  set -- write component properties
  getAllProperties -- enumerate active property IDs

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> Wrapper::get() -> ValueTree property read
