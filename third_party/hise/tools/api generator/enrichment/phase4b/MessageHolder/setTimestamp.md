MessageHolder::setTimestamp(Number timestampSamples) -> undefined

Thread safety: SAFE
Sets the absolute sample timestamp. Clamped to 0..1073741823 (0x3FFFFFFF).
Preserves the upper 2 flag bits (artificial/ignored markers) in the same uint32
field. Unique to MessageHolder -- Message uses delayEvent() for relative offsets.

Pair with:
  getTimestamp -- read current timestamp
  addToTimestamp -- add relative delta (note: currently inaccessible from script)
  Synth.addMessageFromHolder -- uses the timestamp for event positioning

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setTimestamp()
    -> e.setTimeStamp(timestampSamples)
    -> clamps to 0..0x3FFFFFFF, preserves flag bits
