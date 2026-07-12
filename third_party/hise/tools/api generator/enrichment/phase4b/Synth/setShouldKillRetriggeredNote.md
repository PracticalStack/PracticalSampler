Synth::setShouldKillRetriggeredNote(Integer killNote) -> undefined

Thread safety: SAFE -- sets a boolean member on the owner synth, no allocations, no locks, no dispatch.
Controls whether the parent synth automatically kills a voice when a new note-on arrives for the
same pitch while the previous voice is still playing. Default is true (kill retriggered notes).
When disabled, both voices coexist, enabling same-pitch note stacking.

Dispatch/mechanics:
  owner->setKillRetriggeredNote(killNote)
  -> writes to shouldKillRetriggeredNote boolean
  -> evaluated during voice allocation on subsequent note-on events

Pair with:
  playNote / addNoteOn -- the note generation methods whose voice allocation is affected
  noteOffByEventId -- manual voice release when retrigger killing is disabled

Anti-patterns:
  - Do NOT disable retrigger killing without managing note-offs manually -- voices accumulate
    on the same pitch and consume the voice pool, eventually hitting the voice limit.

Source:
  ScriptingApi.cpp  Synth::setShouldKillRetriggeredNote()
    -> owner->setKillRetriggeredNote(killNote)
