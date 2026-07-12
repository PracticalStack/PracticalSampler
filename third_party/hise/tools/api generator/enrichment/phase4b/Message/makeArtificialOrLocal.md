Message::makeArtificialOrLocal() -> Integer

Thread safety: SAFE
Converts the current event to artificial, always creating a new event with a new ID --
even if already artificial. Returns the new event ID. Key difference from makeArtificial()
which is idempotent (returns existing ID for already-artificial events).

Dispatch/mechanics:
  Same as makeArtificial() but skips the "already artificial" early-return check.
  Always registers a new artificial event with a new sequential ID from EventIdHandler.

Anti-patterns:
  - Calling on an already-artificial note-on overwrites the local artificialNoteOnIds
    cache. The previous artificial event ID is lost, which may break note-off matching
    if both IDs need tracking.
  - Always assigns a new event ID -- subsequent Synth.addVolumeFade() or
    Synth.addPitchFade() calls targeting the old ID will no longer reach the voice.

Source:
  ScriptingApi.cpp  Message::makeArtificialInternal(true)
    -> skips isArtificial() early-return
    -> otherwise identical to makeArtificial
