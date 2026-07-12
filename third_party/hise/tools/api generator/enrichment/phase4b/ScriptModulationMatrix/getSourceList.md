ScriptModulationMatrix::getSourceList() -> Array

Thread safety: UNSAFE -- creates a new Array from the internal source list.
Returns an array of strings containing the IDs of all modulation sources. Sources
are the modulator processors in the GlobalModulatorContainer's gain modulation
chain, identified by their processor ID.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Pair with:
  getTargetList -- enumerate targets to pair with sources
  canConnect -- validate source/target pairs

Source:
  ScriptModulationMatrix.cpp  getSourceList()
    -> copies sourceList StringArray (filled at construction from
       MatrixIds::Helpers::fillModSourceList)
