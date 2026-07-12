ScriptTable::setTablePopupFunction(Function newFunction) -> undefined

Thread safety: UNSAFE
Sets custom drag-popup text formatter for table editing.
Function receives x/y and should return string-compatible display text.
Callback signature: newFunction(double x, double y)

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Stores callback in tableValueFunction and sends parameterId property change.
  During drag, TableWrapper::getTextForTablePopup() calls function if valid; otherwise falls back to default "x | y" text.

Pair with:
  setMouseHandlingProperties -- popup behavior and drag interaction are tuned together
  setSnapValues -- shares the same wrapper update trigger path

Anti-patterns:
  - Do NOT assume invalid values throw errors -- non-function values silently revert to default popup text.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::setTablePopupFunction() -> property change message
  HISE/hi_scripting/scripting/api/ScriptComponentWrappers.cpp:1571  TableWrapper::getTextForTablePopup() callback dispatch
  HISE/hi_tools/hi_standalone_components/TableEditor.h:83  default popup string behavior
