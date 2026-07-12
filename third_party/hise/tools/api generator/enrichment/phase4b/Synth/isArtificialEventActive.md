Synth::isArtificialEventActive(Integer eventId) -> Integer

Thread safety: SAFE -- single array lookup into pre-allocated fixed-size array, no allocations, no locks.
Checks whether an artificial event with the given ID is currently active (note-on registered,
not yet released). Only tracks artificial events -- real MIDI events always return false.

Anti-patterns:
  - Do NOT rely on this for very old event IDs in long-running sessions -- the ring buffer
    uses modular indexing (eventId % HISE_EVENT_ID_ARRAY_SIZE), so recycled slots may
    produce false positives.

Source:
  ScriptingApi.cpp  Synth::isArtificialEventActive()
    -> getMainController_()->getEventHandler().isArtificialEventId((uint16)eventId)
