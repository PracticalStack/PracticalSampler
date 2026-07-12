Message::ignoreEvent(Integer shouldBeIgnored) -> undefined

Thread safety: SAFE
Sets or clears the ignored flag (bit 30 of timestamp). When ignored, the event remains
in the buffer but is skipped by downstream processors. Passing truthy sets the flag;
falsy clears it. Requires a mutable callback context.

Dispatch/mechanics:
  messageHolder->ignoreEvent(shouldBeIgnored)
  Special case: if ignoring an artificial note-off whose event ID matches
  artificialNoteOnThatWasKilled (from makeArtificial()), auto-reinserts the matching
  note-on into EventIdHandler and local cache to prevent stuck notes.

Pair with:
  makeArtificial -- often used together: makeArtificial on note-on, ignoreEvent on note-off
  isArtificial -- check whether an event is artificial before deciding to ignore

Anti-patterns:
  - In deferred mode, ignored events are skipped entirely (never reach the callback).
    Calling ignoreEvent(false) to re-enable an event in deferred mode has no effect.
  - The note-on reinsert logic only triggers when all conditions are met: shouldBeIgnored
    is truthy, event is artificial, event is note-off, and event ID matches
    artificialNoteOnThatWasKilled. If makeArtificial() was not called on the note-off,
    reinsert does not occur.

Source:
  ScriptingApi.cpp  Message::ignoreEvent()
    -> null check on messageHolder (unconditional, not guarded by ENABLE_SCRIPTING_SAFE_CHECKS)
    -> conditional reinsert: reinsertArtificialNoteOn() + pushArtificialNoteOn()
    -> messageHolder->ignoreEvent(shouldBeIgnored) sets bit 30 of timestamp
