Engine::getMasterPeakLevel(int channel) -> Double

Thread safety: SAFE -- reads cached display value (outL/outR)
Returns peak volume (0.0 to 1.0) for the given output channel. 0=left, 1=right.
Any channel other than 0 silently returns right channel.
Source:
  ScriptingApi.cpp  Engine::getMasterPeakLevel()
    -> reads main synth chain display values (outL/outR)
