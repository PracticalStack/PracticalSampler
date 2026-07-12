Modulator::setMatrixProperties(JSON matrixData) -> undefined

Thread safety: UNSAFE -- involves JSON parsing, property storage, and
synchronous notification dispatch.
Sets the matrix modulation range properties for a MatrixModulator. Converts JSON
data to RangeData and stores it on the GlobalModulatorContainer's matrix
properties system.

Dispatch/mechanics:
  dynamic_cast<MatrixModulator*>(mod)
    -> MatrixIds::Helpers::Properties::RangeData::fromJSON(matrixData)
    -> stores range data keyed by mm->getMatrixTargetId()
    -> sends property update notification

Anti-patterns:
  - [BUG] Silently does nothing when called on a non-MatrixModulator. No error
    is reported, so the call appears to succeed.

Source:
  ScriptingApiObjects.cpp:3021  setMatrixProperties()
    -> casts to MatrixModulator* -> stores RangeData on GlobalModulatorContainer
