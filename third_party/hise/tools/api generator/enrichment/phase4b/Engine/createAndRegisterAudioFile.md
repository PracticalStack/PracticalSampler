Engine::createAndRegisterAudioFile(int index) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, ExternalDataHolder slot registration
Creates a ScriptAudioFile and registers it at the given slot index for cross-module
access via the ExternalData system. Multiple calls with the same index replace the
previous registration.
Pair with:
  getComplexDataReference -- access data from other modules
  createAndRegisterSliderPackData/createAndRegisterTableData/createAndRegisterRingBuffer
Source:
  ScriptingApi.cpp  Engine::createAndRegisterAudioFile()
    -> new ScriptAudioFile -> ExternalDataHolder::registerAtSlot(index)
