TableProcessor::restoreFromBase64(int tableIndex, String state) -> undefined

Thread safety: UNSAFE
Restores the table's graph points from a base64-encoded string created by
exportAsBase64. Empty string triggers reset to default state. Invalid base64
that decodes to zero bytes is silently ignored (table unchanged).

Required setup:
  const var tp = Synth.getTableProcessor("ModulatorId");

Dispatch/mechanics:
  ExternalDataHolder::getTable(tableIndex) -> Table::restoreData(state)
    -> if empty: Table::reset()
    -> else: base64 decode -> deserialize GraphPoint array -> fillLookUpTable()

Pair with:
  exportAsBase64 -- serialize state for later restoration

Source:
  ScriptingApiObjects.h:2554  ScriptingTableProcessor::restoreFromBase64()
  Delegates to Table::restoreData() in hi_tools/hi_tools/Tables.h
