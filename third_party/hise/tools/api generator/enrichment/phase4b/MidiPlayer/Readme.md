MidiPlayer (object)
Obtain via: Synth.getMidiPlayer("MidiPlayer1")

Script handle to a MIDI Player module for MIDI file playback, recording,
editing, and visualization. Supports loading multiple MIDI sequences,
controlling transport, editing MIDI data with undo, and extracting note
data for piano roll UIs.

Complexity tiers:
  1. MIDI file utility: setFile, getEventList, flushMessageList,
     saveAsMidiFile, connectToPanel, getNoteRectangleList. Load, edit, and
     visualize MIDI data without transport.
  2. Playback engine: + play, stop, getPlaybackPosition,
     setPlaybackCallback, setRepaintOnPositionChange. Real-time MIDI
     playback with transport controls and position-synced UI.
  3. Sequencer with recording: + record, setRecordEventCallback,
     setSyncToMasterClock, connectToMetronome, setUseGlobalUndoManager.
     Full DAW-style sequencer with quantized recording, master clock sync,
     and undo support.
  4. Multi-pattern programmatic sequencer: + create,
     flushMessageListToSequence, setSequence, setTimeSignature,
     setUseTimestampInTicks, getTicksPerQuarter, isSequenceEmpty,
     asMidiProcessor, setGlobalPlaybackRatio,
     setAutomationHandlerConsumesControllerEvents,
     convertEventListToNoteRectangles. Multiple sequences per player,
     programmatic note construction from UI data, CC automation embedding.

Practical defaults:
  - Use setUseTimestampInTicks(true) for any musical editing. Tick
    timestamps (960 per quarter note) are tempo-independent and align to
    musical grid positions. Sample timestamps shift with tempo changes.
  - Call clearAllSequences() before create() when initializing a fresh
    player. create() appends to the sequence list rather than replacing.
  - Initialize multiple pattern banks in a loop: call create(4, 4, 2) N
    times after clearAllSequences(), then setSequence(1) to select the first.
  - Use setFile(file, true, true) (clear + select) as the standard pattern
    for loading a single MIDI file. Only pass false for the clear parameter
    when intentionally accumulating multiple sequences.
  - When building a step sequencer, set
    setAutomationHandlerConsumesControllerEvents(true) so CC messages in the
    MIDI data drive parameter automation during playback.

Common mistakes:
  - Calling play()/stop() when synced to master clock -- they are silent
    no-ops that return false. Use TransportHandler to control transport.
  - Using sample timestamps for grid-aligned editing -- sample values shift
    with tempo. Call setUseTimestampInTicks(true) before getEventList() /
    flushMessageList().
  - Creating sequences without clearing first -- create() appends to the
    sequence list. Call clearAllSequences() before create() in a loop.
  - Using setSequence(0) or setTrack(0) -- sequence and track indices are
    one-based. Index 0 triggers a script error.
  - Calling undo()/redo() without enabling undo --
    setUseGlobalUndoManager(true) must be called first or a script error is
    thrown.
  - Setting Tempo in the time signature object and calling
    setTimeSignature() -- the Tempo property is read-only and ignored.

Example:
  // Get a reference to a MidiPlayer module
  const var mp = Synth.getMidiPlayer("MidiPlayer1");

  // Create an empty 4/4 sequence with 4 bars
  mp.create(4, 4, 4);

  // Start playback
  mp.play(0);

Methods (44):
  asMidiProcessor                    clearAllSequences
  connectToMetronome                 connectToPanel
  convertEventListToNoteRectangles   create
  flushMessageList                   flushMessageListToSequence
  getEventList                       getEventListFromSequence
  getLastPlayedNotePosition          getMidiFileList
  getNoteRectangleList               getNumSequences
  getNumTracks                       getPlaybackPosition
  getPlayState                       getTicksPerQuarter
  getTimeSignature                   getTimeSignatureFromSequence
  isEmpty                            isSequenceEmpty
  play                               record
  redo                               reset
  saveAsMidiFile                     setAutomationHandlerConsumesControllerEvents
  setFile                            setGlobalPlaybackRatio
  setPlaybackCallback                setPlaybackPosition
  setRecordEventCallback             setRepaintOnPositionChange
  setSequence                        setSequenceCallback
  setSyncToMasterClock               setTimeSignature
  setTimeSignatureToSequence         setTrack
  setUseGlobalUndoManager            setUseTimestampInTicks
  stop                               undo
