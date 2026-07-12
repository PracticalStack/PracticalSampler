Sampler::setTimestretchRatio(Double newRatio) -> undefined

Thread safety: UNSAFE -- modifies sampler timestretch state
Sets the timestretch ratio. 1.0 = original speed, 2.0 = double speed, 0.5 =
half speed. Timestretch mode must be enabled via setTimestretchOptions() first.
Pair with:
  setTimestretchOptions -- must enable a timestretch mode first
  getTimestretchOptions -- read current configuration
Source:
  ScriptingApi.cpp  Sampler::setTimestretchRatio()
