Message::makeArtificial() -> Integer

Thread safety: SAFE
Converts the current event to artificial (script-owned), disconnecting it from the
original MIDI input. Returns the event ID. Idempotent: if already artificial, returns
the existing ID without creating a duplicate.

Dispatch/mechanics:
  For note-on: sets artificial flag, pushes to global EventIdHandler (assigns new ID)
    and local artificialNoteOnIds cache. Replaces original event in buffer via swapWith.
  For note-off: sets artificial flag, pops matching note-on from EventIdHandler using
    cached ID, stores it in artificialNoteOnThatWasKilled (for ignoreEvent reinsert),
    assigns matching note-on's ID to this note-off. If no match found, note-off is ignored.
  For other types: sets artificial flag only.

Pair with:
  getEventId -- read the assigned ID after making artificial
  ignoreEvent -- often paired for suppress-and-resynthesize patterns
  makeArtificialOrLocal -- non-idempotent variant (always creates new ID)

Anti-patterns:
  - On note-off, if makeArtificial() was never called on the corresponding note-on,
    the note-off is automatically ignored (no matching note-on found) and won't trigger
    voice release.
  - Resets artificialNoteOnThatWasKilled at the start of every call -- only the most
    recent call's note-on is preserved for ignoreEvent reinsert.

Source:
  ScriptingApi.cpp  Message::makeArtificialInternal(false)
    -> if already artificial: return existing eventId (early exit)
    -> copy.setArtificial()
    -> EventIdHandler::pushArtificialNoteOn(copy) for note-on
    -> EventIdHandler::popNoteOnFromEventId() for note-off
    -> copy.swapWith(*messageHolder)
