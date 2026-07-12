# MidiPlayer -- Class Analysis

## Brief
Script handle to a MIDI Player module for MIDI file playback, recording, editing, and visualization.

## Purpose
MidiPlayer provides scripting access to a MIDI Player processor module in the signal chain. It supports loading and managing multiple MIDI sequences, controlling playback/recording transport, editing MIDI data with undo support, and extracting note data for UI visualization. The player can sync to the master clock for host-aligned playback, and supports callbacks for playback state changes, sequence modifications, and real-time event filtering during recording.

## Details

### Architecture

MidiPlayer wraps a core `MidiPlayer` DSP processor (a `MidiProcessor` + `TempoListener`). The scripting object (`ScriptedMidiPlayer`) inherits from `MidiPlayerBaseType` (which registers as a `SequenceListener`), `ConstScriptingObject`, and `SuspendableTimer`.

### Multi-Sequence Model

The player can hold multiple MIDI sequences simultaneously. Each sequence (`HiseMidiSequence`) contains multiple tracks (internally `MidiMessageSequence` objects). Selection is one-based in the scripting API:

| Concept | Internal | Scripting API |
|---------|----------|---------------|
| Sequence index | 0-based (`currentSequenceIndex`) | 1-based (`setSequence(1)` = first) |
| Track index | 0-based (`currentTrackIndex`) | 1-based (`setTrack(1)` = first) |

The special index -1 for `getSequenceWithIndex()` returns the current sequence. Index 0 in the scripting API triggers a script error.

### Timestamp Modes

Event timestamps can use two formats, controlled by `setUseTimestampInTicks()`:

| Mode | Default | Unit | Context |
|------|---------|------|---------|
| Samples | Yes | Sample count at current SR/BPM | Natural for audio-aligned processing |
| Ticks | No | MIDI ticks (960 per quarter note) | Natural for music-aligned editing |

This affects `getEventList()`, `getEventListFromSequence()`, `flushMessageList()`, and `flushMessageListToSequence()`.

### Transport State Machine

Three states: Stop (0), Play (1), Record (2). State transitions via `play()`, `stop()`, `record()`.

When `syncToMasterClock` is enabled, `play()` and `stop()` become no-ops (return false). Transport is driven by the master clock via `onGridChange()` and `onTransportChange()`. `record()` from stop defers to the next clock start.

### Callback System

| Callback | Registration | Threading | Arguments |
|----------|-------------|-----------|-----------|
| Sequence callback | `setSequenceCallback(fn)` | Async (message thread) | `fn(midiPlayer)` |
| Playback callback (async) | `setPlaybackCallback(fn, 0)` | Deferred (UI thread) | `fn(timestamp, playState)` |
| Playback callback (sync) | `setPlaybackCallback(fn, 1)` | Audio thread | `fn(timestamp, playState)` |
| Record event callback | `setRecordEventCallback(fn)` | Audio thread | `fn(messageHolder)` |

The record event callback and synchronous playback callback run on the audio thread and require realtime-safe callable objects.

### Undo System

Undo is **disabled by default** (`undoManager = nullptr`). Call `setUseGlobalUndoManager(true)` to enable undo tracking via the global undo manager (`Engine.undo()`). Undoable operations: `flushMessageList`, `flushMessageListToSequence`, `setTimeSignature`, `setTimeSignatureToSequence`, sequence loading via `setFile`.

### Playback Speed

Per-player speed is set via MidiProcessor attributes (`PlaybackSpeed`). Global speed ratio (`setGlobalPlaybackRatio()`) multiplies all MidiPlayer playback speeds.

### Time Signature Object

The JSON object used by `getTimeSignature()` / `setTimeSignature()`:

| Property | Type | Description |
|----------|------|-------------|
| Nominator | double | Numerator of time signature |
| Denominator | double | Denominator of time signature |
| NumBars | double | Number of bars |
| LoopStart | double | Normalised loop start (0.0-1.0) |
| LoopEnd | double | Normalised loop end (0.0-1.0) |
| Tempo | double | BPM (read-only -- not consumed by setTimeSignature) |

## obtainedVia
`Synth.getMidiPlayer(playerId)`

## minimalObjectToken
mp

## Constants

(No constants registered on this class.)

## Dynamic Constants

(None.)

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `mp.undo();` without enabling undo | `mp.setUseGlobalUndoManager(true);` then edit, then `mp.undo();` | Undo is disabled by default. Calling undo() without enabling it first throws a script error. |
| `mp.setSequence(0);` | `mp.setSequence(1);` | Sequence and track indices are one-based. Index 0 triggers a script error. |
| Modifying Tempo in time signature object and calling setTimeSignature | Set Tempo via the module attribute instead | setTimeSignature does not consume the Tempo property from the JSON object -- it is read-only. |

## codeExample
```javascript
// Get a reference to a MidiPlayer module
const var mp = Synth.getMidiPlayer("MidiPlayer1");

// Create an empty 4/4 sequence with 4 bars
mp.create(4, 4, 4);

// Start playback
mp.play(0);
```

## Alternatives
MidiProcessor -- for generic MIDI processor attribute access (use `mp.asMidiProcessor()` to get one from a MidiPlayer handle).

## Related Preprocessors
None.

## Diagrams

### transport-states
- **Brief:** Transport State Machine
- **Type:** state
- **Description:** Three states: Stop (0), Play (1), Record (2). play() transitions Stop->Play or Record->Play. stop() transitions Play->Stop or Record->Stop (with finishRecording). record() transitions Stop->Record or Play->Record. When syncToMasterClock is true, play() and stop() are no-ops; transport is driven by onGridChange/onTransportChange from the master clock. record() from Stop defers via recordOnNextPlaybackStart flag.
