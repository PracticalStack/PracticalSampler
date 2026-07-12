ScriptTable::setMouseHandlingProperties(JSON propertyObject) -> undefined

Thread safety: UNSAFE
Configures drag interaction behavior by forwarding a property object to TableEditor::MouseDragProperties.
Controls swap behavior, edge locks, snap configuration, point sizes, wheel curve editing, and path closing.

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  ScriptTable broadcasts propertyObject via dragProperties LambdaBroadcaster.
  TableWrapper listener forwards object to TableEditor::setMouseDragProperties() -> MouseDragProperties::fromVar.

Pair with:
  setSnapValues -- explicit snap positions used by drag quantization
  setTablePopupFunction -- tune drag UX when changing editor behavior
  setTablePoint -- script-side point edits complement drag rules

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::setMouseHandlingProperties() broadcaster trigger
  HISE/hi_scripting/scripting/api/ScriptComponentWrappers.cpp:1571  TableWrapper listener bridge
  HISE/hi_tools/hi_standalone_components/TableEditor.cpp:36  MouseDragProperties::fromVar parser and drag behavior
