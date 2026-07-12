MessageHolder::setType(Number type) -> undefined

Thread safety: SAFE
Sets the event type. Validated against 0..13 -- values outside this range trigger a
script error ("Unknown Type: N"). A newly created MessageHolder has type Empty (0),
which must be changed before Synth.addMessageFromHolder() will accept it. Unique to
MessageHolder -- Message has no type setter.

Required setup:
  const var mh = Engine.createMessageHolder();
  mh.setType(mh.NoteOn);  // use constants: Empty(0), NoteOn(1), NoteOff(2),
                           // Controller(3), PitchBend(4), Aftertouch(5),
                           // AllNotesOff(6), SongPosition(7), MidiStart(8),
                           // MidiStop(9), VolumeFade(10), PitchFade(11),
                           // TimerEvent(12), ProgramChange(13)

Anti-patterns:
  - Do NOT forget to set the type on newly created holders -- default is Empty,
    and Synth.addMessageFromHolder() rejects Empty events with a script error.

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setType()
    -> isPositiveAndBelow(type, (int)HiseEvent::Type::numTypes)
    -> e.setType((HiseEvent::Type)type)
    -> reportScriptError if out of range
