Sampler::getReleaseStartOptions() -> JSON

Thread safety: UNSAFE -- allocates JSON object
Returns the current release start options as a JSON object. Requires
HISE_SAMPLER_ALLOW_RELEASE_START preprocessor (enabled by default).
Returns: {ReleaseFadeTime: int, FadeGamma: double, UseAscendingZeroCrossing: bool,
  GainMatchingMode: string, PeakSmoothing: double}
Pair with:
  setReleaseStartOptions -- configure release start behavior
  setAllowReleaseStart -- enable/disable per event
Source:
  ScriptingApi.cpp  Sampler::getReleaseStartOptions()
    -> ReleaseStartOptions::toJSON()
