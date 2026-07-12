ScriptModulationMatrix::getMatrixModulationProperties() -> JSON

Thread safety: UNSAFE -- creates a new JSON object via the container's toJSON method.
Returns a JSON object containing the current global matrix modulation properties,
including selectable sources mode, default init values, and range properties.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Pair with:
  setMatrixModulationProperties -- set/modify the properties this method reads

Source:
  ScriptModulationMatrix.cpp  getMatrixModulationProperties()
    -> container->matrixProperties.toJSON()
