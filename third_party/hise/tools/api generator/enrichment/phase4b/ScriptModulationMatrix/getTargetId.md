ScriptModulationMatrix::getTargetId(NotUndefined componentOrId) -> String

Thread safety: UNSAFE -- looks up components by name, involves string construction and dynamic_cast.
Returns the modulation target ID for the given UI component. Accepts either a
direct component reference or a component name string. Returns an empty string
if the component is not found or has no target ID.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  Resolves component reference (by object or name string)
    -> For components connected to MatrixModulator Value: returns modulator processor ID
    -> For ScriptSliders with matrixTargetId: returns that property value

Pair with:
  getComponent -- reverse lookup: target ID -> component
  getTargetList -- enumerate all available target IDs

Source:
  ScriptModulationMatrix.cpp  getTargetId()
    -> resolves component -> checks MatrixModulator connection or matrixTargetId property
