ScriptModulationMatrix::getTargetList() -> Array

Thread safety: UNSAFE -- creates a new Array from the internal target list.
Returns an array of strings containing the IDs of all modulation targets. Targets
come from MatrixModulator processors (identified by processor ID or custom target
ID) and ScriptSlider components with a non-empty matrixTargetId property.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Pair with:
  getSourceList -- enumerate sources to pair with targets
  getComponent -- get the UI component for a target ID

Source:
  ScriptModulationMatrix.cpp  getTargetList()
    -> copies allTargets StringArray (filled at construction from
       MatrixIds::Helpers::fillModTargetList with TargetType::All)
