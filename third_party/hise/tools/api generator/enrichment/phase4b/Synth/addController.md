Synth::addController(Integer channel, Integer number, Integer value, Integer timeStampSamples) -> undefined

Thread safety: SAFE -- creates a HiseEvent on the stack and inserts into MIDI buffer, no allocations, no locks.
Adds a controller event to the MIDI buffer with explicit channel and sample-accurate timestamp.
The event is marked as artificial. Supports standard CC (0-127), pitch bend (128), and aftertouch (129).

Pair with:
  sendController -- same effect but no channel/timestamp control and does NOT set artificial flag

Anti-patterns:
  - Do NOT assume sendController and addController produce identical events -- addController
    sets the artificial flag, sendController does not. Downstream logic filtering on the
    artificial flag will treat them differently.

Source:
  ScriptingApi.cpp:5276+  Synth::addController()
    -> HiseEvent(Type::Controller, number, value, channel)
    -> e.setArtificial()
    -> addHiseEventToBuffer(e)
