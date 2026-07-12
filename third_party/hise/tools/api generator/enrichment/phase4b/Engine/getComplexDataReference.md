Engine::getComplexDataReference(String dataType, String moduleId, int index) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, processor tree lookup
Returns a scripting reference to complex data (Table, SliderPack, AudioFile, DisplayBuffer)
owned by another module. dataType must be "Table", "SliderPack", "AudioFile", or "DisplayBuffer".
Anti-patterns:
  - Do NOT pass "FilterCoefficients" -- passes validation but silently returns undefined
  - Do NOT assume a valid return -- check isDefined() since out-of-range index returns undefined silently
Source:
  ScriptingApi.cpp  Engine::getComplexDataReference()
    -> ProcessorHelpers::getFirstProcessorWithName(moduleId)
    -> ExternalDataHolder::getComplexData(type, index)
