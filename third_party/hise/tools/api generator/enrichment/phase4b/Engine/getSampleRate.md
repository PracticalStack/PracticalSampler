Engine::getSampleRate() -> Double

Thread safety: SAFE
Returns the current audio sample rate in Hz (e.g., 44100.0, 48000.0).
Pitfall: returns -1 during onInit in exported plugins (audio engine not yet initialized).
Defer sample-rate logic to a timer or control callback.
Source:
  ScriptingApi.h  inline -> getProcessor()->getSampleRate()
