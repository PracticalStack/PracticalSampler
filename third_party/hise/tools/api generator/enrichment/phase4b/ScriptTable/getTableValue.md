ScriptTable::getTableValue(Number inputValue) -> Number

Thread safety: UNSAFE
Returns interpolated table output for a normalized input position.
Uses the currently bound table source (owned, processor slot, or referred object).

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  dynamic_cast to SampleLookupTable -> getInterpolatedValue(inputValue, sendNotificationAsync)
  if cast fails, returns 0.0 silently

Pair with:
  setTablePoint -- shape the curve that getTableValue reads
  addTablePoint -- add more breakpoints before interpolation

Anti-patterns:
  - Do NOT pass raw MIDI values (eg. 64 or 100) -- normalize to 0..1 first.
  - Do NOT assume any bound table type is valid -- non-SampleLookupTable sources return 0.0 silently.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::getTableValue() -> SampleLookupTable cast and interpolation
  HISE/hi_tools/hi_tools/Tables.h:50  SampleLookupTable interpolation API
