Sampler::getTimestretchOptions() -> JSON

Thread safety: UNSAFE -- allocates JSON object
Returns the current timestretch configuration as a JSON object.
Returns: {Mode: string, Tonality: double, SkipLatency: bool,
  NumQuarters: double, PreferredEngine: string}
Mode values: "Disabled", "VoiceStart", "TimeVariant", "TempoSynced"
Pair with:
  setTimestretchOptions -- configure timestretch behavior
  setTimestretchRatio -- set the stretch ratio
Source:
  ScriptingApi.cpp  Sampler::getTimestretchOptions()
    -> TimestretchOptions::toJSON()
