Sampler::setAllowReleaseStart(Integer eventId, Integer shouldBeAllowed) -> Integer

Thread safety: SAFE
Enables or disables release start for a specific event. Returns true on success.
Requires HISE_SAMPLER_ALLOW_RELEASE_START preprocessor (enabled by default).
Pair with:
  getReleaseStartOptions -- get current release start configuration
  setReleaseStartOptions -- configure release start behavior globally
Source:
  ScriptingApi.cpp  Sampler::setAllowReleaseStart()
