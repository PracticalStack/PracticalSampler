MessageHolder::isController() -> Integer

Thread safety: SAFE
Returns true if this event is a CC, PitchBend, OR Aftertouch event. This broadened
definition matches the HISE convention where the onController callback receives all
three event types. Unique to MessageHolder -- Message does not need this because the
callback type already identifies the event.

Dispatch/mechanics:
  e.isController() || e.isPitchWheel() || e.isAftertouch()
  Returns true for event types 3 (Controller), 4 (PitchBend), 5 (Aftertouch).

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::isController()
    -> e.isController() || e.isPitchWheel() || e.isAftertouch()
