Engine::createAndRegisterRingBuffer(int index) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, ExternalDataHolder slot registration
Creates a ScriptRingBuffer (circular display buffer for oscilloscope/FFT visualizations)
and registers it at the given slot index for cross-module access via ExternalData.
Multiple calls with the same index replace the previous registration.
Pair with:
  getComplexDataReference -- access data from other modules
  createAndRegisterAudioFile/createAndRegisterSliderPackData/createAndRegisterTableData
Source:
  ScriptingApi.cpp  Engine::createAndRegisterRingBuffer()
    -> new ScriptRingBuffer -> ExternalDataHolder::registerAtSlot(index)
