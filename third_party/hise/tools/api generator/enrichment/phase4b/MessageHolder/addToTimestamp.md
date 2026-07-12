MessageHolder::addToTimestamp(Number deltaSamples) -> undefined

Thread safety: SAFE
Adds a delta in samples to the current timestamp. Clamped to minimum 0 internally.
The delta is cast to int16 before application, limiting effective range to -32768..32767.

Pair with:
  setTimestamp -- set absolute timestamp
  getTimestamp -- read current timestamp

Anti-patterns:
  - [BUG] This method is not registered in the scripting API constructor (missing
    ADD_API_METHOD_1). It appears in documentation but is inaccessible from HISEScript.
    Use setTimestamp(getTimestamp() + delta) as a workaround.
  - Do NOT pass delta values outside -32768..32767 -- the int16 cast silently truncates,
    potentially shifting the timestamp in the wrong direction.

Source:
  ScriptingApiObjects.cpp:5683  ScriptingMessageHolder::addToTimestamp()
    -> HiseEvent::addToTimeStamp((int16)deltaSamples)
    -> clamps result to >= 0
