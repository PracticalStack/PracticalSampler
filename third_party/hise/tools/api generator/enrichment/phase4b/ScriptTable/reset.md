ScriptTable::reset() -> undefined

Thread safety: UNSAFE
Resets table data to the default linear ramp from (0,0) to (1,1).

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Direct delegation to cached Table reset path, then editor/view updates follow data notifications.

Pair with:
  addTablePoint -- rebuild custom shapes after reset
  setTablePoint -- edit default points after reset

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::reset() -> Table reset path
  HISE/hi_tools/hi_tools/Tables.cpp:35  Table reset and graph recalculation
