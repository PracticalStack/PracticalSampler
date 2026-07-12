DspNetwork::prepareToPlay(Double sampleRate, Double blockSize) -> undefined

Thread safety: UNSAFE -- acquires a write lock on the connection lock (if already initialized). Calls prepare() and reset() on the root node, which may allocate buffers and initialize DSP state.
Initializes the network for audio processing at the given sample rate and block size.
Runs pending post-init functions, prepares the root node, and marks the network as
initialized. If sampleRate is zero or negative, returns without doing anything.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
Pair with:
  processBlock -- processes audio after initialization
Anti-patterns:
  - Do NOT skip calling prepareToPlay before processBlock -- processing silently
    returns without modifying buffers, no error thrown.
Source:
  DspNetwork.cpp:745  prepareToPlay()
    -> runs pending post-init functions
    -> acquires write lock on connectionLock (if already initialized)
    -> adjusts blockSize via DynamicParameterModulationProperties
    -> getRootNode()->prepare(currentSpecs) with voiceIndex from polyHandler
    -> getRootNode()->reset()
    -> sets initialised = true
