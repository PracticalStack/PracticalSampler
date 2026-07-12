Engine::createAndRegisterSliderPackData(int index) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, ExternalDataHolder slot registration
Creates a ScriptSliderPackData (array of float values for slider pack UI) and registers
it at the given slot index for cross-module access via ExternalData. Multiple calls
with the same index replace the previous registration.
Pair with:
  getComplexDataReference -- access data from other modules
  createAndRegisterAudioFile/createAndRegisterTableData/createAndRegisterRingBuffer
Source:
  ScriptingApi.cpp  Engine::createAndRegisterSliderPackData()
    -> new ScriptSliderPackData -> ExternalDataHolder::registerAtSlot(index)
