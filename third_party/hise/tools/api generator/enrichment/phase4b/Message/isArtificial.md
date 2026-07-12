Message::isArtificial() -> Integer

Thread safety: SAFE
Returns whether the current event was created by a script or internal HISE mechanism
rather than from external MIDI input. The artificial flag is bit 31 of the timestamp.
Returns false silently if called outside a MIDI callback (no error reported).

Pair with:
  makeArtificial -- convert an event to artificial
  makeArtificialOrLocal -- convert with forced new ID

Source:
  ScriptingApi.cpp  Message::isArtificial()
    -> if constMessageHolder is null, returns 0 silently (no error)
    -> constMessageHolder->isArtificial()
    -> checks bit 31 of timestamp field
