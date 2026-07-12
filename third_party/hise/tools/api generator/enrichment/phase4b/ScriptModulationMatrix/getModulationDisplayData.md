ScriptModulationMatrix::getModulationDisplayData(String targetId) -> JSON

Thread safety: UNSAFE -- accesses slider state, creates DynamicObject, may cache query functions.
Returns a JSON object containing modulation visualization data for the specified
target. Results are cached after the first call per target. Returns undefined if
no matching target is found.

Returned properties:
  valueNormalized    The slider's current normalized value (0-1)
  scaledValue        Value after scale-mode modulation is applied
  addValue           Cumulative additive modulation offset
  modulationActive   Whether any modulation connections are active
  modMinValue        Lower bound of the modulation range
  modMaxValue        Upper bound of the modulation range
  lastModValue       Previous modulation output value

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  Looks up target in queryFunctions cache (creates on first call)
    -> MatrixModulator targets: ModulationDisplayValue::QueryFunction from modulator
    -> Cable targets: query function from script processor
    -> getModulationDataFromQueryFunction() -> storeToJSON()

Pair with:
  getComponent -- get the UI component for the same target
  getTargetList -- enumerate valid target IDs

Source:
  ScriptModulationMatrix.cpp  getModulationDisplayData()
    -> queryFunctions map lookup/insert -> getModulationDataFromQueryFunction()
    -> ModulationDisplayValue::storeToJSON()
