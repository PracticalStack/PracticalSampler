TableProcessor::getTable(int tableIndex) -> ScriptObject

Thread safety: UNSAFE
Returns a Table data object wrapping the same underlying table data. The Table
object provides a richer API: getTableValueNormalised() for curve evaluation,
setDisplayCallback() for ruler change notifications, and point editing methods
without the tableIndex parameter. Changes through the Table object are
reflected in the parent module and vice versa.

Required setup:
  const var tp = Synth.getTableProcessor("ModulatorId");

Dispatch/mechanics:
  checkValidObject() -> dynamic_cast<ProcessorWithExternalData*>(processor)
    -> new ScriptTableData(scriptProcessor, tableIndex, externalDataHolder)

Pair with:
  addTablePoint/setTablePoint -- Table object has its own versions without tableIndex
  reset -- Table object has its own reset()

Source:
  ScriptingApiObjects.cpp:5032+  creates ScriptTableData (scripting API class "Table")
  ScriptingApiObjects.h:1250  ScriptTableData : ScriptComplexDataReferenceBase
