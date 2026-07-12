ScriptModulationMatrix::setConnectionProperty(String sourceId, String targetId, String propertyId, NotUndefined value) -> Integer

Thread safety: UNSAFE -- ValueTree setProperty with undo manager, triggers property listeners.
Sets the value of a specific property on the connection between the given source
and target. The change is recorded in the undo manager. Returns true if
successful, false if source not found, no connection exists, or invalid property.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");
  mm.connect("LFO1", "CutoffMod", true);

Pair with:
  getConnectionProperty -- read back the property value
  connect -- connection must exist before setting properties

Source:
  ScriptModulationMatrix.cpp  setConnectionProperty()
    -> finds connection ValueTree child
    -> c.setProperty(id, value, getControlUndoManager())
