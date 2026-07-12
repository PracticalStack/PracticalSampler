Modulator::getAttribute(Number parameterIndex) -> Double

Thread safety: SAFE
Returns the current value of a modulator attribute at the given parameter index.

Pair with:
  setAttribute -- set the value at the same index
  getAttributeId -- get the name for a given index

Source:
  ScriptingApiObjects.cpp  getAttribute() -> mod->getAttribute(parameterIndex)
