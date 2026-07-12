ScriptModulationMatrix::getConnectionProperty(String sourceId, String targetId, String propertyId) -> NotUndefined

Thread safety: UNSAFE -- ValueTree property access with internal locking.
Returns the value of a specific property on the connection between the given
source and target. Returns undefined if the source is not found, no connection
exists, or the property ID is invalid.

Valid propertyId values:
  "SourceIndex"    Index into source list (-1 = disconnected)
  "TargetId"       The target identifier string
  "Intensity"      Modulation depth [-1.0, 1.0]
  "Mode"           0 = Scale, 1 = Unipolar, 2 = Bipolar
  "Inverted"       Whether modulation signal is inverted (boolean)
  "AuxIndex"       Secondary source index (-1 = none)
  "AuxIntensity"   Secondary source modulation depth

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");
  mm.connect("LFO1", "CutoffMod", true);

Pair with:
  setConnectionProperty -- write the property value
  connect -- connection must exist before querying properties

Anti-patterns:
  - Do NOT use undefined return to distinguish error cases -- invalid source,
    missing connection, and invalid property all return undefined identically.

Source:
  ScriptModulationMatrix.cpp  getConnectionProperty()
    -> finds sourceIndex -> iterates matrixData children for matching connection
    -> checks propertyId against getWatchableIds() -> returns ValueTree property
