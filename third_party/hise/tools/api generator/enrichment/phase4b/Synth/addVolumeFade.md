Synth::addVolumeFade(Integer eventId, Integer fadeTimeMilliseconds, Integer targetVolume) -> undefined

Thread safety: SAFE -- creates a HiseEvent on the stack and inserts into MIDI buffer, no allocations, no locks. The auto-kill path also pops a note-on and inserts a note-off, all lock-free.
Applies a volume fade (in dB) to an active voice by event ID. When targetVolume is exactly -100,
triggers "fade to silence and kill" -- creates both volume fade and automatic note-off at fade end.

Dispatch/mechanics:
  HiseEvent::createVolumeFade(eventId, fadeTimeMs, targetVolume)
  -> inherits current event's timestamp
  Special path when targetVolume == -100:
    -> pops note-on from EventHandler via popNoteOnFromEventId()
    -> creates NoteOff with timestamp = (1.0 + fadeTimeMs/1000.0 * sampleRate) samples
    -> only works on artificial events (real events produce script error)

Pair with:
  playNote / addNoteOn -- the note must be playing to apply a volume fade
  noteOffByEventId -- alternative: immediate note-off without fade

Anti-patterns:
  - Do NOT use addVolumeFade with -100 on real (non-artificial) events -- produces script
    error "Hell breaks loose if you kill real events artificially!".

Source:
  ScriptingApi.cpp  Synth::addVolumeFade()
    -> HiseEvent::createVolumeFade(eventId, fadeTimeMs, (uint8)targetVolume)
    -> if targetVolume == -100: popNoteOnFromEventId() + auto note-off
