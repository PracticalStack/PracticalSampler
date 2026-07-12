Synth::sendController(Integer number, Integer value) -> undefined

Thread safety: SAFE -- creates a HiseEvent on the stack and inserts into MIDI buffer, no allocations, no locks.
Sends a controller event with the timestamp of the current event. Supports standard CC (0-127),
pitch bend (128, value 0-16383), and aftertouch (129). Does NOT set the artificial flag.

Anti-patterns:
  - The event is NOT marked artificial (unlike addController). If downstream logic filters
    on the artificial flag, sendController and addController behave differently.
  - No explicit channel parameter -- uses default channel. Use addController for channel control.

Pair with:
  addController -- variant with explicit channel, timestamp, and artificial flag

Source:
  ScriptingApi.cpp  Synth::sendController()
    -> branch on number: 128=PitchBend, 129=Aftertouch, else CC
    -> inherits timestamp from current event
    -> addHiseEventToBuffer()
