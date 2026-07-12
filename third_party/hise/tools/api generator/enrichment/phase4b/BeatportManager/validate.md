BeatportManager::validate() -> JSON

Thread safety: UNSAFE -- blocks the calling thread (1.5s simulated delay in development mode, SDK call in production). Performs file I/O and JSON parsing in simulation mode. Extends the script engine timeout to prevent watchdog termination.
Validates the current Beatport access and returns the result as a JSON object.
In simulation mode, waits 1.5s, then reads and parses validate_response.json.
Throws a script error if the file is missing or contains invalid JSON.

Required setup:
  const bp = Engine.createBeatportManager();
  bp.setProductId("my-product-id");

Dispatch/mechanics:
  HISE_INCLUDE_BEATPORT=1: pimpl->validate() -- delegates to Beatport SDK
  HISE_INCLUDE_BEATPORT=0: Thread::wait(1500) -> reads validate_response.json -> JSON::parse()
  Both paths: extendTimeout(elapsed) to prevent script watchdog termination

Pair with:
  setProductId -- must set product ID before validating
  isBeatportAccess -- quick boolean check before full validation

Anti-patterns:
  - Do NOT call validate() in simulation mode without creating
    {project}/AdditionalSourceCode/beatport/validate_response.json first --
    throws a script error

Source:
  ScriptExpansion.cpp:3452  BeatportManager::validate()
    -> Thread::getCurrentThread()->wait(1500) [simulation]
    -> getBeatportProjectFolder().getChildFile("validate_response.json")
    -> JSON::parse(fileContent)
    -> extendTimeout(elapsed)
