ScriptTable::setSnapValues(Array snapValueArray) -> undefined

Thread safety: UNSAFE
Sets x-axis snap targets used by table point dragging.
Values are expected in normalized 0..1 space.

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Validates isArray() for script error reporting, then stores snapValues and sends parameterId property change.
  Wrapper receives property update and applies snap list to TableEditor.

Pair with:
  setMouseHandlingProperties -- snapWidth and numSteps control snap capture behavior
  setTablePopupFunction -- same property-change trigger path updates popup behavior state

Anti-patterns:
  - Do NOT pass non-array values -- script reports an error, but internal update path still runs and can confuse debugging.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::setSnapValues() -> property change message
  HISE/hi_scripting/scripting/api/ScriptComponentWrappers.cpp:1571  TableWrapper::updateComponent() -> TableEditor::setSnapValues()
