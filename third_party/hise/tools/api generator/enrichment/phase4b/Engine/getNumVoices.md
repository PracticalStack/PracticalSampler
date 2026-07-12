Engine::getNumVoices() -> Integer

Thread safety: SAFE -- reads activeVoices.size() from synth chain children
Returns total currently active voices across all synthesisers. Not the max polyphony.
Source:
  ScriptingApi.cpp  Engine::getNumVoices()
    -> iterates child synths, sums active voice counts
