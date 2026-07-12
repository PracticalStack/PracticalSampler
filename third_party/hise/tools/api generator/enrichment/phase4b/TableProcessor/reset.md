TableProcessor::reset(int tableIndex) -> undefined

Thread safety: UNSAFE
Resets the table to its default state: a straight diagonal line from (0,0) to
(1,1) with curve 0.5. All existing graph points are removed and replaced with
two default endpoints. Lookup table is recalculated and UI update is sent.

Required setup:
  const var tp = Synth.getTableProcessor("ModulatorId");

Dispatch/mechanics:
  ExternalDataHolder::getTable(tableIndex) -> Table::reset()
    -> clears GraphPoint array, adds (0,0,0.5) and (1,1,0.5)
    -> fillLookUpTable() + sendChangeMessage()

Pair with:
  addTablePoint -- always call reset before building a new curve shape
  restoreFromBase64 -- restoring empty string is equivalent to reset()

Source:
  ScriptingApiObjects.h:2554  ScriptingTableProcessor::reset()
  Delegates to Table::reset() in hi_tools/hi_tools/Tables.h
