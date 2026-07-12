TableProcessor::addTablePoint(int tableIndex, Double x, Double y) -> undefined

Thread safety: UNSAFE
Adds a new graph point at (x, y) with default curve 0.5 (linear). The point is
appended to the internal graph point array, the lookup table is recalculated,
and a UI update is sent.

Required setup:
  const var tp = Synth.getTableProcessor("ModulatorId");

Dispatch/mechanics:
  dynamic_cast<ExternalDataHolder*>(processor)->getTable(tableIndex)
    -> Table::addTablePoint(x, y, 0.5f)
    -> fillLookUpTable() + sendChangeMessage()

Pair with:
  reset -- always reset before building a new curve shape
  setTablePoint -- adjust curve value on the added point (addTablePoint always uses 0.5)

Anti-patterns:
  - Do NOT call without reset() first -- points accumulate on existing ones,
    producing unpredictable curves
  - Do NOT rely on addTablePoint for precise curve shapes -- curve is always
    0.5 (linear). Call setTablePoint() afterward to set a custom curve value.
  - [BUG] Unlike setTablePoint, x and y are NOT clamped to 0..1. Out-of-range
    values are stored as-is and may produce unexpected table shapes.

Source:
  ScriptingApiObjects.h:2554  ScriptingTableProcessor class declaration
  ScriptingApiObjects.cpp:5032+  delegates to Table::addTablePoint(x, y, 0.5f)
