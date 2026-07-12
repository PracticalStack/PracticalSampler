Message::getTimestamp() -> Integer

Thread safety: SAFE
Returns the sample-accurate timestamp of the current MIDI event -- the sample offset from
the start of the current audio buffer. Artificial and ignored flag bits are masked out.
Events with timestamps beyond the buffer size are automatically deferred to a future buffer.

Pair with:
  delayEvent -- adds to the timestamp to delay event processing

Source:
  ScriptingApi.cpp  Message::getTimestamp()
    -> constMessageHolder->getTimeStamp()
    -> masks with 0x0FFFFFFF to strip flag bits (artificial bit 31, ignored bit 30)
