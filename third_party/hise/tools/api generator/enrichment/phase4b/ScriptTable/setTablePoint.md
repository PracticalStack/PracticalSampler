ScriptTable::setTablePoint(Integer pointIndex, Number x, Number y, Number curve) -> undefined

Thread safety: UNSAFE
Updates one existing graph point in the table.
x/y/curve are clamped to valid normalized ranges by the table-side point edit path.

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Delegates to cached Table point-update call.
  First and last point x positions remain anchored at 0 and 1.

Pair with:
  addTablePoint -- add points before editing them
  getTableValue -- validate resulting transfer curve behavior

Anti-patterns:
  - Do NOT rely on out-of-range pointIndex validation -- invalid indices are ignored silently.
  - Do NOT expect edge-point x movement -- first and last points keep fixed x positions.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::setTablePoint() -> cached Table edit
  HISE/hi_tools/hi_tools/Tables.cpp:35  point clamping and endpoint constraints
