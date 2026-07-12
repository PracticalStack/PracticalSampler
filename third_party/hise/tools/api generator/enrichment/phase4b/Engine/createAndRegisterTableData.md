Engine::createAndRegisterTableData(int index) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, ExternalDataHolder slot registration
Creates a ScriptTableData (lookup table curve for table UI) and registers it at the
given slot index for cross-module access via ExternalData. Multiple calls with the
same index replace the previous registration.
Pair with:
  getComplexDataReference -- access data from other modules
  createAndRegisterAudioFile/createAndRegisterSliderPackData/createAndRegisterRingBuffer
Source:
  ScriptingApi.cpp  Engine::createAndRegisterTableData()
    -> new ScriptTableData -> ExternalDataHolder::registerAtSlot(index)
