Sampler::setGUISelection(Array sampleList, Integer addToSelection) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, USE_BACKEND only, MessageManagerLock
Sets the sample editor GUI selection to the given array of Sample objects.
Only works in the HISE IDE. Silently does nothing in exported plugins.
Pair with:
  createListFromGUISelection -- read the current GUI selection
  createSelection -- create Sample arrays to pass to this method
Source:
  ScriptingApi.cpp  Sampler::setGUISelection()
    -> guarded by #if USE_BACKEND
