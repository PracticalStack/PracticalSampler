TableProcessor::exportAsBase64(int tableIndex) -> String

Thread safety: UNSAFE
Serializes the table's graph points (positions and curve values) to a
base64-encoded string for later restoration with restoreFromBase64.

Required setup:
  const var tp = Synth.getTableProcessor("ModulatorId");

Dispatch/mechanics:
  ExternalDataHolder::getTable(tableIndex) -> Table::exportData()
    -> serializes GraphPoint array to MemoryBlock -> base64 encode

Pair with:
  restoreFromBase64 -- restore the serialized state

Anti-patterns:
  - Returns an empty string for the default table state (diagonal 0,0 to 1,1)
    rather than a base64 representation. This is intentional --
    restoreFromBase64 handles empty strings by calling reset().

Source:
  ScriptingApiObjects.h:2554  ScriptingTableProcessor::exportAsBase64()
  Delegates to Table::exportData() in hi_tools/hi_tools/Tables.h
