Engine::setLatencySamples(int latency) -> undefined

Thread safety: UNSAFE -- notifies host of latency change (may trigger host callbacks)
Sets the plugin's reported latency in samples for host delay compensation.
Pair with:
  getLatencySamples -- read back the current value
Source:
  ScriptingApi.cpp  Engine::setLatencySamples()
    -> AudioProcessor::setLatencySamples(latency)
