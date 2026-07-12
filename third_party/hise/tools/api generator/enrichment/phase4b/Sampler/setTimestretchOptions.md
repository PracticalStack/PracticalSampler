Sampler::setTimestretchOptions(JSON newOptions) -> undefined

Thread safety: UNSAFE -- allocates TimestretchOptions, modifies sampler state
Sets the timestretch configuration from a JSON object.
Mode values: "Disabled" (no stretching), "VoiceStart" (new voices use current
ratio), "TimeVariant" (all voices share ratio), "TempoSynced" (ratio from
sample length and tempo).
Properties: {Mode: string, Tonality: double (0.0-1.0),
  SkipLatency: bool, NumQuarters: double, PreferredEngine: string}
Pair with:
  getTimestretchOptions -- read current configuration
  setTimestretchRatio -- set the stretch ratio
Source:
  ScriptingApi.cpp  Sampler::setTimestretchOptions()
    -> TimestretchOptions::fromJSON(newOptions)
