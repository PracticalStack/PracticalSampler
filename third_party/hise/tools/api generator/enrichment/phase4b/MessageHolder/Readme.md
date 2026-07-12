MessageHolder (object)
Obtain via: Engine.createMessageHolder()

Persistent MIDI event container that stores a HiseEvent by value. Unlike the
transient Message object (valid only during a callback), MessageHolder persists
across callback boundaries and can be passed to Synth.addMessageFromHolder(),
stored in arrays, used with UnorderedStack event-stack mode, or round-tripped
through MidiPlayer and File MIDI APIs.

Constants:
  EventType:
    Empty = 0              Uninitialized event type
    NoteOn = 1             Note-on event type
    NoteOff = 2            Note-off event type
    Controller = 3         MIDI CC event type
    PitchBend = 4          Pitch bend event type
    Aftertouch = 5         Aftertouch event type (both mono and poly)
    AllNotesOff = 6        All-notes-off event type
    SongPosition = 7       Song position pointer event type
    MidiStart = 8          MIDI start event type
    MidiStop = 9           MIDI stop event type
    VolumeFade = 10        Internal volume fade event type
    PitchFade = 11         Internal pitch fade event type
    TimerEvent = 12        Timer callback event type
    ProgramChange = 13     Program change event type

Complexity tiers:
  1. Store and re-inject: Message.store(), setGain, setVelocity,
     Synth.addMessageFromHolder(). Intercept events, store them, re-inject
     modified copies later.
  2. Sequence construction: + setType, setNoteNumber, setChannel, setTimestamp,
     setControllerNumber, setControllerValue. Build MIDI patterns from scratch
     for MidiPlayer or offline rendering.
  3. Event list processing: + isNoteOn, getNoteNumber, getTimestamp, dump.
     Iterate MidiPlayer.getEventList() arrays to manipulate existing MIDI data
     for export, conversion, or analysis.

Practical defaults:
  - Always set the type before any other field. A default-constructed
    MessageHolder has type Empty, which Synth.addMessageFromHolder() rejects.
  - Use Message.store(holder) rather than manually copying each field. It
    captures all event data in one call.
  - When building NoteOn/NoteOff pairs for MidiPlayer, create separate
    MessageHolder objects per event. Pushing the same reference multiple times
    means all array slots point to the same event.
  - For note tracking, combine MessageHolder with
    UnorderedStack.setIsEventStack(true, stack.EventId).
  - Set channel to 1 (not 0) for standard MIDI. Default channel is 0, which
    is outside the MIDI 1-16 range.

Common mistakes:
  - Reusing one MessageHolder in a loop and pushing to an array -- all slots
    share the same reference. Create a new holder or call clone() per event.
  - Calling Synth.addMessageFromHolder() without setting the type first --
    "Event is empty" error on default-constructed holders.
  - Setting timestamp to 0 for all events in a MidiPlayer sequence -- all
    events fire simultaneously on the first beat.
  - Setting channel to 0 instead of 1 -- channel 0 is outside standard MIDI
    range and may produce unexpected routing behavior.

Example:
  const var mh = Engine.createMessageHolder();
  mh.setType(mh.NoteOn);
  mh.setNoteNumber(60);
  mh.setVelocity(100);
  mh.setChannel(1);

Methods (37):
  addToTimestamp                    clone
  dump                             getChannel
  getCoarseDetune                  getControllerNumber
  getControllerValue               getEventId
  getFineDetune                    getGain
  getMonophonicAftertouchPressure  getNoteNumber
  getPolyAfterTouchNoteNumber      getPolyAfterTouchPressureValue
  getTimestamp                     getTransposeAmount
  getVelocity                      ignoreEvent
  isController                     isMonophonicAfterTouch
  isNoteOff                        isNoteOn
  isPolyAftertouch                 setChannel
  setCoarseDetune                  setControllerNumber
  setControllerValue               setFineDetune
  setGain                          setMonophonicAfterTouchPressure
  setNoteNumber                    setPolyAfterTouchNoteNumberAndPressureValue
  setStartOffset                   setTimestamp
  setTransposeAmount               setType
  setVelocity
