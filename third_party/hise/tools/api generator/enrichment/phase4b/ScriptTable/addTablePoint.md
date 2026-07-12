ScriptTable::addTablePoint(Number x, Number y) -> undefined

Thread safety: UNSAFE
Adds a new graph point to the table data and recalculates the table.
The point is appended to the current point list.

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Delegates to the cached Table object in ComplexDataScriptComponent.
  Mutation goes through Table point-update paths protected by table-side locking.

Pair with:
  setTablePoint -- edit an existing point after insertion
  reset -- return to linear default shape before rebuilding points

Anti-patterns:
  - Do NOT pass x or y outside 0..1 -- addTablePoint does not clamp in the Table layer and can create unexpected curve shapes.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::addTablePoint() -> cached Table mutation
  HISE/hi_tools/hi_tools/Tables.cpp:35  Table point storage and recalculation path
