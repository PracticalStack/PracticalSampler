ScriptModulationMatrix::getComponent(String targetId) -> ScriptObject

Thread safety: UNSAFE -- iterates all UI components with dynamic_cast checks.
Returns the UI component (ScriptSlider or other ScriptComponent) associated with
the given modulation target ID. Returns undefined if no matching component is found.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  For MatrixModulator targets: finds the component connected to the modulator's
    Value parameter via AttributeListener lookup
  For parameter targets: finds the ScriptSlider with matching matrixTargetId property

Pair with:
  getTargetId -- reverse lookup: component -> target ID
  getTargetList -- get valid target IDs first

Source:
  ScriptModulationMatrix.cpp  getComponent()
    -> iterates ScriptingContent components with dynamic_cast
    -> checks MatrixModulator Value connection or matrixTargetId property match
