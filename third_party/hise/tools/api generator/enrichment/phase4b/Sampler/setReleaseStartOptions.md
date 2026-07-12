Sampler::setReleaseStartOptions(JSON data) -> undefined

Thread safety: UNSAFE -- allocates ReleaseStartOptions object
Sets the release start options from a JSON object. Requires
HISE_SAMPLER_ALLOW_RELEASE_START preprocessor (enabled by default).
Properties: {ReleaseFadeTime: int (0-44100, default 4096),
  FadeGamma: double (0.0-2.0, default 1.0),
  UseAscendingZeroCrossing: bool (default false),
  GainMatchingMode: "None"|"Volume"|"Offset",
  PeakSmoothing: double (default 0.96)}
Pair with:
  getReleaseStartOptions -- read current configuration
  setAllowReleaseStart -- enable/disable per event
Source:
  ScriptingApi.cpp  Sampler::setReleaseStartOptions()
    -> ReleaseStartOptions::fromJSON(data)
