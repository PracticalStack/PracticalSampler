MessageHolder::getEventId() -> Integer

Thread safety: SAFE
Returns the event ID (uint16, 0-65535, wraps at 65536). Default is 0 for newly
created holders. When re-injected via Synth.addMessageFromHolder(), the system
assigns a new ID for NoteOn events -- the stored ID is not used.

Pair with:
  Synth.addMessageFromHolder -- re-injection assigns new event IDs
  Message.store -- captures the live event's current ID

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getEventId()
    -> (int)e.getEventId()
