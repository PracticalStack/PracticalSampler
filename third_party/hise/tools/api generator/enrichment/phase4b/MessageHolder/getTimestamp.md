MessageHolder::getTimestamp() -> Integer

Thread safety: SAFE
Returns the sample-accurate timestamp. Stored in the lower 28 bits of a uint32 field
(upper 4 bits reserved for artificial/ignored flags). Represents a sample offset from
the buffer start, or an absolute position in MidiPlayer event lists depending on the
timestamp edit format.

Pair with:
  setTimestamp -- set absolute timestamp
  addToTimestamp -- add relative delta

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getTimestamp()
    -> (int)e.getTimeStamp()
