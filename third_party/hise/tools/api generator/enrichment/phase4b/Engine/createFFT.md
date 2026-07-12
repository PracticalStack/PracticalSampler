Engine::createFFT() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates an FFT object for frequency-domain analysis on audio buffers, providing
forward and inverse FFT operations.
Source:
  ScriptingApi.cpp  Engine::createFFT()
    -> new ScriptFFT
