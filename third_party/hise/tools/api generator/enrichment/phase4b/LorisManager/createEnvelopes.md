LorisManager::createEnvelopes(ScriptObject file, String parameter, Integer harmonicIndex) -> Array

Thread safety: UNSAFE -- allocates heap buffers for envelope data and creates VariantBuffer objects per channel.
Creates an array of audio-rate Buffer objects (one per channel) representing the
envelope of the specified parameter for a given harmonic index. Each buffer is
sampled at the audio file's sample rate. Pass 0 for harmonicIndex to get the
fundamental.
parameter: "rootFrequency", "frequency", "phase", "gain", or "bandwidth".
The "rootFrequency" parameter uses F0 estimate internally; all others trigger
prepareToMorph (channelization, collation, sifting, distillation, sorting).

Required setup:
  const var lm = Engine.getLorisManager();
  lm.analyse(audioFile, 440.0);

Dispatch/mechanics:
  initThreadController() -> LorisManager::createEnvelope()
    -> loris_create_envelope() via C API (or createF0Estimate for "rootFrequency")
    -> returns VariantBuffer per channel at original sample rate

Pair with:
  analyse -- file must be analysed first
  createEnvelopePaths -- returns Path objects instead of raw Buffers
  createSnapshot -- returns values at a single time point instead of full envelope

Anti-patterns:
  - Do NOT pass a non-File object -- silently returns an empty array.

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::createEnvelopes()
    -> LorisManager::createEnvelope() -> loris_create_envelope() via C API
