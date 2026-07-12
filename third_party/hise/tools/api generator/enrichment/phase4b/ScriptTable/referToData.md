ScriptTable::referToData(ScriptObject tableData) -> undefined

Thread safety: UNSAFE
Rebinds this table component to another table data source.
Accepts ScriptTableData, compatible complex-data component sources, or -1 to restore local owned data.

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Delegates to ComplexDataScriptComponent::referToDataBase(Table, tableData).
  Updates holder/index routing, then wrapper source watcher rebinds TableEditor to new data.

Pair with:
  registerAtParent -- create ScriptTableData handles to share with other components

Anti-patterns:
  - Do NOT pass unsupported object types -- call is ignored and previous data source stays active.
  - Do NOT pass mismatched complex-data types -- type mismatch prevents table binding.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::referToData() -> referToDataBase()
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3137  ComplexDataScriptComponent source-selection and rebinding
