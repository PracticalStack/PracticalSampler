---
title: MIDI Player
moduleId: MidiPlayer
type: MidiProcessor
subtype: MidiProcessor
tags: [sequencing, generator]
builderPath: b.MidiProcessors.MidiPlayer
screenshot: /images/v2/reference/audio-modules/midiplayer.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: MidiMetronome, type: companion, reason: "Generates metronome clicks synchronised to this player's position and time signature" }
commonMistakes:
  - title: "Must load MIDI file first"
    wrong: "Pressing play without loading a MIDI file first"
    right: "Load a MIDI file via the scripting API or drag-and-drop overlay, then call play()"
    explanation: "With no sequence loaded, CurrentSequence is 0 (no selection) and playback produces no output."
  - title: "One-shot mode ignores LoopStart"
    wrong: "Setting LoopStart to create a play region in one-shot mode"
    right: "Enable looping if you need the playback region to start at LoopStart"
    explanation: "In one-shot mode, playback always starts from the beginning and stops at LoopEnd. LoopStart only takes effect when LoopEnabled is on."
  - title: "MIDI files use host tempo"
    wrong: "Expecting the MIDI file to play at its embedded tempo"
    right: "Set the host tempo to match the desired playback speed"
    explanation: "Playback is always synchronised to the host tempo. The MIDI file's embedded tempo is discarded on load. Use PlaybackSpeed to scale relative to the host."
  - title: "NoteOff events invisible to sibling scripts"
    wrong: "Placing a script processor as a sibling of the MIDI Player and expecting it to receive noteOff callbacks"
    right: "Place the script processor inside a child container beneath the MIDI Player"
    explanation: "NoteOff events are pushed to the future-event buffer and are not processed by sibling MIDI processors in the same chain."
  - title: "MIDI files missing from compiled plugin"
    wrong: "Compiling without exporting pooled MIDI files first"
    right: "Run Export > Export Pooled Files To Binary Resource before compiling"
    explanation: "MIDI files must be explicitly exported to the binary resource pool. If skipped, the compiled plugin contains an empty MIDI pool and no sequences will play."
forumReferences:
  - id: 1
    title: "NoteOff events from MidiPlayer are invisible to sibling MIDI script processors"
    summary: "Script processors placed as siblings of the MidiPlayer in the same MIDI chain will not receive noteOff callbacks; place them inside a child container beneath the MidiPlayer instead."
    topic: 1903
  - id: 2
    title: "MIDI files missing from compiled plugins if pooled files not exported first"
    summary: "MIDI files must be exported via 'Export Pooled Files To Binary Resource' before compiling; skipping this step results in an empty MIDI pool in the compiled plugin."
    topic: 9646
  - id: 3
    title: "Sequence length defaults to last note-off, not bar boundary"
    summary: "Use setSequenceCallback with Math.ceil on NumBars to normalize the loop boundary to full bars after loading any MIDI file."
    topic: 7311
customEquivalent:
  approach: hisescript
  moduleType: "ScriptProcessor"
  complexity: complex
  description: "Requires manual tick-to-sample conversion, host tempo tracking, loop wrap-around logic, and artificial note generation via Message.makeArtificial(). Recording adds further complexity."
llmRef: |
  MidiPlayer (MidiProcessor)

  Plays back MIDI sequences synchronised to host tempo. Loads standard MIDI files from the pool and injects note, CC, and pitch wheel events into the MIDI buffer with sample-accurate timing. Supports looping, one-shot playback, variable speed, multi-track selection, and overdub recording.

  Signal flow:
    [MIDI file sequence] -> fetch events for current tick range -> convert to sample-accurate events -> MIDI buffer out
    Incoming MIDI passes through; captured only during recording.

  CPU: low (event fetch is a sequential scan, max 16 events per block), monophonic, constant cost.

  Parameters:
    CurrentPosition (0-100%, default 0%) - normalised playback position, read-write (seeking supported), not saved with presets
    CurrentSequence (0-16, default 0) - 1-based sequence index; 0 = no sequence selected
    CurrentTrack (1-16, default 1) - selects which track of a multi-track MIDI file to play and edit
    LoopEnabled (Off/On, default On) - toggles looping vs one-shot playback
    LoopStart (0-100%, default 0%) - loop region start; only used when looping is enabled
    LoopEnd (0-100%, default 100%) - playback endpoint in both loop and one-shot modes
    PlaybackSpeed (0.01-16.0, default 1.0) - speed multiplier on top of host tempo sync

  Transport: controlled via play() / stop() / record() from the scripting API. Optionally syncs to the master clock.

  When to use:
    Playing back pre-composed MIDI sequences (melodies, arpeggios, drum patterns) in sync with the host. Provides loop regions, multi-track selection, and recording/overdub.

  Common mistakes:
    Must load a MIDI file before playback - CurrentSequence defaults to 0 (no selection).
    LoopStart has no effect in one-shot mode.
    MIDI files play at host tempo, not their embedded tempo.
    NoteOff events are invisible to sibling script processors - place scripts in a child container.
    MIDI files must be exported to binary resource pool before compiling or they will be missing.

  Tips:
    All MidiPlayer events are artificial - use Message.isArtificial() to distinguish from live input.
    Sequence length defaults to last note-off, not bar boundary - use setSequenceCallback with Math.ceil to normalise.
    Only one TimeSignature per sequence is supported.
    Pass empty string to setFile() to clear all sequences.
    The built-in keyboard FloatingTile does not animate from MidiPlayer output - use a custom panel with getNoteRectangleList().

  See also: MidiMetronome (companion - generates clicks from this player's position).
---

::category-tags
---
tags:
  - { name: sequencing, desc: "MIDI processors that generate or play back note sequences" }
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs. Contains modulators and MIDI processors." }
---
::

![MIDI Player screenshot](/images/v2/reference/audio-modules/midiplayer.png)

The MIDI Player loads standard MIDI files and plays them back as sample-accurate MIDI events synchronised to the host tempo. It injects note-on, note-off, CC, and pitch wheel events into the MIDI buffer from the selected sequence and track. Incoming MIDI passes through unmodified unless recording is active, in which case live input is captured into the sequence.

Playback supports looping with adjustable loop boundaries, one-shot mode (play once and stop), variable speed relative to the host tempo, and multi-track selection for MIDI files containing multiple tracks. Transport is controlled via the scripting API (`play()`, `stop()`, `record()`) or optionally synchronised to the master clock. MIDI files are loaded from the HISE pool system and can also be dragged onto the overlay in the editor.

## Signal Path

::signal-path
---
glossary:
  parameters:
    CurrentPosition:
      desc: "Normalised playback position (read-write; setting it seeks)"
      range: "0 - 100%"
      default: "0%"
    CurrentSequence:
      desc: "1-based sequence index; 0 means no sequence selected"
      range: "0 - 16"
      default: "0"
    CurrentTrack:
      desc: "Selects which track of a multi-track MIDI file to play and edit"
      range: "1 - 16"
      default: "1"
    LoopEnabled:
      desc: "Toggles between looping and one-shot playback"
      range: "Off / On"
      default: "On"
    LoopStart:
      desc: "Loop region start position (only used when looping)"
      range: "0 - 100%"
      default: "0%"
    LoopEnd:
      desc: "Playback endpoint in both loop and one-shot modes"
      range: "0 - 100%"
      default: "100%"
    PlaybackSpeed:
      desc: "Speed multiplier applied on top of host tempo sync"
      range: "0.01 - 16.0"
      default: "1.0"
  functions:
    calculateTickRange:
      desc: "Converts the current audio block size to a tick range using host BPM and playback speed"
    fetchEvents:
      desc: "Retrieves all MIDI events from the sequence that fall within the tick range (max 16 per block)"
    wrapTickRange:
      desc: "Splits the tick range into before-wrap and after-wrap sub-ranges when it crosses the loop boundary"
    injectEvent:
      desc: "Converts a sequence event to a sample-accurate MIDI message and adds it to the output buffer"
---

```
// MidiPlayer - tempo-synced MIDI sequence playback
// MIDI file in -> MIDI events out (incoming MIDI passes through)

if not playing or CurrentSequence == 0:
    pass through incoming MIDI
    return

// Calculate tick range for this audio block
tickRange = calculateTickRange(hostBPM, PlaybackSpeed, blockSize)

// Fetch and inject events from the selected sequence and track
events = fetchEvents(CurrentSequence, CurrentTrack, tickRange)

if tickRange crosses LoopEnd:
    wrapTickRange(LoopStart, LoopEnd)
    // fetch events from the wrapped portion too

for each event in events:
    injectEvent(event)    // noteOn, noteOff, CC, pitch wheel

// Advance position
advance CurrentPosition

if CurrentPosition > LoopEnd:
    if LoopEnabled:
        CurrentPosition = LoopStart    // wrap around
    else:
        stop playback                  // one-shot: stop

// Recording (when in record state)
// Incoming live MIDI is captured with tick timestamps
// and merged into the current sequence
```

::

## Parameters

::parameter-table
---
groups:
  - label: Sequence Selection
    params:
      - { name: CurrentSequence, desc: "Selects which loaded MIDI sequence to play. Value is 1-based: 1 = first sequence, 2 = second, etc. A value of 0 means no sequence is selected and playback produces no output.", range: "0 - 16", default: "0" }
      - { name: CurrentTrack, desc: "Selects which track of a multi-track MIDI file to play and edit. Affects both the events generated during playback and the track displayed in editor overlays.", range: "1 - 16", default: "1" }
  - label: Transport
    params:
      - { name: CurrentPosition, desc: "Normalised playback position. Reading it returns the current position; setting it seeks to that point within the loop region. This parameter is not saved with presets.", range: "0 - 100%", default: "0%" }
      - { name: PlaybackSpeed, desc: "Speed multiplier applied on top of host tempo synchronisation. At 1.0, a 4-bar sequence plays over exactly 4 bars at the host tempo. At 2.0, it plays in half the time. Also multiplied by the global playback speed.", range: "0.01 - 16.0", default: "1.0" }
  - label: Loop Region
    params:
      - { name: LoopEnabled, desc: "When on, playback wraps from LoopEnd back to LoopStart. When off, playback runs once from the start to LoopEnd and then stops.", range: "Off / On", default: "On" }
      - { name: LoopStart, desc: "Start of the loop region. When looping, playback wraps to this position after reaching LoopEnd. Has no effect in one-shot mode.", range: "0 - 100%", default: "0%" }
      - { name: LoopEnd, desc: "End of the playback region. Acts as the endpoint in both looping and one-shot modes.", range: "0 - 100%", default: "100%" }
---
::

### Tempo Synchronisation

Playback is always synchronised to the host tempo. The embedded tempo in MIDI files is discarded on load; all timing is recalculated relative to the host BPM. The internal tick resolution is 960 ticks per quarter note. Tempo map events such as ritardando or accelerando embedded in a MIDI file are ignored entirely [1]($FORUM_REF.1425$) [2]($FORUM_REF.7411$). Use `Engine.setHostBpm()` or the PlaybackSpeed parameter for tempo adjustments in standalone mode.

### Sequence Length and Time Signatures

When a MIDI file is loaded, its length is set to the timestamp of the last note-off event rather than the last bar boundary. If the file has no end-of-track marker or no notes reaching the final bar, the reported length may be fractional [3]($FORUM_REF.7311$) [4]($FORUM_REF.5595$). Call `setSequenceCallback` with a function that reads the TimeSignature, rounds NumBars up with `Math.ceil`, and writes it back via `setTimeSignature` to ensure the loop boundary falls on a bar line [5]($FORUM_REF.7311$).

Each sequence supports exactly one TimeSignature. Files with internal time signature changes will have those changes ignored [6]($FORUM_REF.11082$). Split content into multiple sequences and switch between them if mixed metres are needed.

### Loop Region Behaviour

LoopStart and LoopEnd define the playback region. In one-shot mode (LoopEnabled off), LoopStart is ignored -- playback runs from the beginning to LoopEnd and stops. In loop mode, the event fetcher handles wrap-around transparently, splitting the tick range at the loop boundary so events near the loop point are not dropped. Note-off events at the wrap boundary are skipped to prevent orphaned releases.

### Artificial Events

All events injected by the MIDI Player are marked as artificial. In a script processor that receives both player output and live keyboard input, use `Message.isArtificial()` to distinguish them [7]($FORUM_REF.13471$). Alternatively, assign the MIDI Player to a different MIDI channel via the CurrentTrack parameter.

The built-in keyboard FloatingTile responds only to incoming MIDI events, not to the artificial events from the MIDI Player. To visualise playback on a keyboard, create a custom panel and use the event callbacks or `getNoteRectangleList` [8]($FORUM_REF.13704$).

### NoteRectangleList Indexing

`getNoteRectangleList()` returns one rectangle per note, following note-on order. If the EventList contains CC or other non-note events, the indices diverge. Filter the EventList to note-on events only before using EventId to index into the rectangle list [9]($FORUM_REF.7724$).

### Clearing Sequences

There is no dedicated method to clear all loaded sequences. Pass an empty string as the first argument to `setFile()` -- for example `MidiPlayer.setFile("", true, true)` -- to effectively clear all sequences [10]($FORUM_REF.4228$).

### Event Limit and Bypass Behaviour

The player generates a maximum of 16 events per audio block. Very dense sequences may have events silently dropped if more than 16 fall within a single block.

When bypassed, note-on events are suppressed but CC and pitch wheel events still pass through.

CurrentPosition is not saved with presets -- it is purely runtime state. When playback starts, the position resets to the beginning of the sequence (or the loop region).

### Editor Overlays

The MidiOverlayPanel FloatingTile hosts several editor overlays for the MIDI Player: a note viewer, CC viewer, loop-based editor, and drag-and-drop file loader. Overlays connect to any MidiPlayer in the module tree and support real-time editing with undo/redo.

**See also:** $MODULES.MidiMetronome$ -- Generates metronome clicks synchronised to this player's position and time signature
