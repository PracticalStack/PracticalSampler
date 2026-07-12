Engine::getLatencySamples() -> Integer

Thread safety: SAFE -- reads cached integer
Returns the plugin's reported latency in samples (for host delay compensation).
Pair with:
  setLatencySamples -- set the latency value
Source:
  ScriptingApi.cpp  Engine::getLatencySamples()
    -> AudioProcessor::getLatencySamples()
