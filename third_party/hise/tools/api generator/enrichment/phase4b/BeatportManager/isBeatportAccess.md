BeatportManager::isBeatportAccess() -> Integer

Thread safety: UNSAFE -- blocks the calling thread (500ms simulated delay in development mode, SDK call in production). Extends the script engine timeout to prevent watchdog termination.
Returns whether the current session has valid Beatport access. In simulation
mode, waits 500ms and returns true if validate_response.json exists.

Required setup:
  const bp = Engine.createBeatportManager();

Dispatch/mechanics:
  HISE_INCLUDE_BEATPORT=1: pimpl->isBeatportAccess() -- delegates to Beatport SDK
  HISE_INCLUDE_BEATPORT=0: Thread::wait(500) -> checks validate_response.json existence
  Both paths: extendTimeout() to prevent script watchdog termination

Pair with:
  validate -- call isBeatportAccess first to check access, then validate for full result
  setProductId -- must set product ID before access checks are meaningful

Source:
  ScriptExpansion.cpp:3485  BeatportManager::isBeatportAccess()
    -> Thread::getCurrentThread()->wait(500) [simulation]
    -> extendTimeout(elapsed)
    -> returns getBeatportProjectFolder().getChildFile("validate_response.json").existsAsFile()
