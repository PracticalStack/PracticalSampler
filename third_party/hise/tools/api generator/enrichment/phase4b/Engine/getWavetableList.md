Engine::getWavetableList() -> Array

Thread safety: UNSAFE -- processor tree traversal, string array allocation
Returns string array of wavetable names from the first WavetableSynth in the chain.
Anti-patterns:
  - Do NOT call without a WavetableSynth in the signal chain -- throws a script error
  - Only queries the first WavetableSynth found; multiple WavetableSynths are not supported
Source:
  ScriptingApi.cpp  Engine::getWavetableList()
    -> ProcessorHelpers::getFirstProcessorWithType<WavetableSynth>()
