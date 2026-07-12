LorisManager::createEnvelopePaths(ScriptObject file, String parameter, Integer harmonicIndex) -> Array

Thread safety: UNSAFE -- calls createEnvelopes internally, then creates Path objects with heap allocations.
Creates an array of Path objects (one per audio channel) representing the envelope
of the specified parameter for a given harmonic index. Downsampled to ~200 display
points and clipped to the valid parameter range. Pass 0 for harmonicIndex to get
the fundamental.
parameter: "rootFrequency", "frequency", "phase", "gain", or "bandwidth".

Required setup:
  const var lm = Engine.getLorisManager();
  lm.analyse(audioFile, 440.0);

Dispatch/mechanics:
  createEnvelopes() -> for each buffer: LorisManager::setEnvelope()
    -> downsamples (3 * size / 600 px), clips to parameter range
    -> wraps each JUCE Path in ScriptingObjects::PathObject

Pair with:
  analyse -- file must be analysed first
  createEnvelopes -- returns raw Buffer data instead of Paths

Anti-patterns:
  - Do NOT pass a non-File object -- silently returns an empty array.

Source:
  ScriptLorisManager.cpp:145-170  ScriptLorisManager::createEnvelopePaths()
    -> createEnvelopes() -> LorisManager::setEnvelope() -> Path creation
