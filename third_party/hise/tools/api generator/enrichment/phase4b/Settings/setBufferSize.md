Settings::setBufferSize(Integer newBlockSize) -> undefined

Thread safety: UNSAFE -- reconfigures audio device buffer size through the device manager
Sets the audio buffer size in samples. The value should be one of the sizes returned
by getAvailableBufferSizes(). Does nothing if no device manager is available.
Primarily useful in standalone builds.

Pair with:
  getAvailableBufferSizes -- list valid buffer sizes
  getCurrentBufferSize -- read the active buffer size

Source:
  ScriptingApi.cpp  Settings::setBufferSize()
    -> driver->setCurrentBlockSize(newBlockSize)
