Message (namespace)
Obtain via: Globally available as `Message` in MIDI callbacks (onNoteOn, onNoteOff, onController). Not user-created.

Transient MIDI event accessor for reading and modifying note, controller, and event
properties inside callbacks. Wraps HISE's internal 16-byte HiseEvent struct. The Message
object is a singleton per script processor -- its internal pointer is only valid during
callback execution. Setter methods modify the event in-place in the audio buffer.

Constants:
  EventType:
    NoteOn = 1             Note-on event type
    NoteOff = 2            Note-off event type
    Controller = 3         MIDI CC event type
    PitchBend = 4          Pitch bend event type
    Aftertouch = 5         Aftertouch event type (both mono and poly)
    AllNotesOff = 6        All-notes-off event type
    VolumeFade = 10        Internal volume fade event type
    PitchFade = 11         Internal pitch fade event type
  VirtualCC:
    PITCH_BEND_CC = 128    Virtual CC number for pitch wheel in onController
    AFTERTOUC_CC = 129     Virtual CC number for aftertouch in onController (historical typo)

Complexity tiers:
  1. Basic MIDI routing: getNoteNumber, getVelocity, getControllerNumber, getControllerValue,
     ignoreEvent. Keyswitch detection, CC parameter changes, note filtering, velocity gating.
  2. Event manipulation: + setTransposeAmount, setCoarseDetune, setVelocity, delayEvent,
     setStartOffset. Timbre shifting, velocity curves, humanization, sample phase control.
  3. Artificial event system: + makeArtificial, getEventId, isArtificial, with
     Synth.addNoteOn/addVolumeFade/addPitchFade. Monophonic scripts, arpeggiators,
     note-splitting, event replacement.
  4. Persistent event storage: + store with MessageHolder. Tracking held notes across
     callbacks, release triggering, advanced sustain pedal logic.

Practical defaults:
  - Use getControllerNumber() with Message.PITCH_BEND_CC (128) and Message.AFTERTOUC_CC
    (129) to handle all controller-type events in a single onController callback.
  - Pair setTransposeAmount(-N) with setCoarseDetune(N) for timbre shifting: transpose
    selects a different sample, coarse detune cancels the pitch change. Opposite signs.
  - Use isArtificial() to guard delayEvent() so humanization only applies to
    sequencer-generated events, not live MIDI input.
  - Use ignoreEvent(true) in onNoteOn for keyswitch notes rather than filtering in
    onController -- keyswitches are note events.
  - Use reg variables (not var) for state that persists across callbacks in the same
    script processor (lastNote, lastEventId, pedalDown).

Common mistakes:
  - Normalizing getControllerValue() by dividing by 127 for all events -- pitch bend
    returns 0-16383, producing values up to ~129.
  - Calling getNoteNumber() inside onController without checking event type first --
    triggers a script error for controller events.
  - Calling setVelocity() inside onNoteOff -- only works on NoteOn events.
  - Reading Message properties outside a MIDI callback -- use Message.store() to copy
    into a MessageHolder for deferred access.
  - Using setTransposeAmount(-5) without setCoarseDetune(5) -- note sounds 5 semitones
    lower instead of selecting a different sample at the same pitch.
  - Applying delayEvent() to all notes unconditionally -- guard with isArtificial() so
    live MIDI input is not delayed.

Example:
  // Message is globally available in MIDI callbacks -- no creation needed.
  // In onNoteOn:
  function onNoteOn()
  {
      local ch = Message.getChannel();
      local note = Message.getNoteNumber();
      local vel = Message.getVelocity();

      if (vel < 20)
          Message.setVelocity(20); // Enforce minimum velocity
  }

Methods (38):
  delayEvent                          getChannel
  getCoarseDetune                     getControllerNumber
  getControllerValue                  getEventId
  getFineDetune                       getGain
  getMonophonicAftertouchPressure     getNoteNumber
  getPolyAfterTouchNoteNumber         getPolyAfterTouchPressureValue
  getProgramChangeNumber              getTimestamp
  getTransposeAmount                  getVelocity
  ignoreEvent                         isArtificial
  isMonophonicAfterTouch              isPolyAftertouch
  isProgramChange                     makeArtificial
  makeArtificialOrLocal               sendToMidiOut
  setAllNotesOffCallback              setChannel
  setCoarseDetune                     setControllerNumber
  setControllerValue                  setFineDetune
  setGain                             setMonophonicAfterTouchPressure
  setNoteNumber                       setPolyAfterTouchNoteNumberAndPressureValue
  setStartOffset                      setTransposeAmount
  setVelocity                         store
