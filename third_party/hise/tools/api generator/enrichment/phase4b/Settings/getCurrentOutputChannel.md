Settings::getCurrentOutputChannel() -> Integer

Thread safety: UNSAFE -- queries audio device for active output channel bits
Returns the index of the currently active output channel pair (stereo pair index,
not an individual channel number). Returns 0 if no audio device is active.

Pair with:
  getAvailableOutputChannels -- list available stereo pairs
  setOutputChannel -- select a different output pair

Source:
  ScriptingApi.cpp  Settings::getCurrentOutputChannel()
    -> queries device active output channels bit pattern
