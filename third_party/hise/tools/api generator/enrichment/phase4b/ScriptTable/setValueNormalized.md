ScriptTable::setValueNormalized(Double normalizedValue) -> undefined

Thread safety: SAFE
Sets the value using normalized input through the base ScriptComponent path.

Dispatch/mechanics:
  Base implementation forwards normalizedValue directly to setValue(normalizedValue).

Pair with:
  getValueNormalized -- read normalized state
  setValue -- raw value setter used internally by base path

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent::setValueNormalized() -> setValue()
