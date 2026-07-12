# MidiPlayer -- Method Analysis

## asMidiProcessor

**Signature:** `ScriptObject asMidiProcessor()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptingMidiProcessor wrapper object (heap allocation).
**Minimal Example:** `var proc = {obj}.asMidiProcessor();`

**Description:**
Returns a typed MidiProcessor reference for the underlying MIDI Player module. This allows access to generic MidiProcessor methods like `setAttribute()` and `getAttribute()` for controlling module parameters (CurrentPosition, CurrentSequence, CurrentTrack, LoopEnabled, LoopStart, LoopEnd, PlaybackSpeed).

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiProcessor.setAttribute$`
- `$API.MidiProcessor.getAttribute$`

---

## clearAllSequences

**Signature:** `undefined clearAllSequences()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls clearSequences with sendNotificationAsync, which modifies the sequence list and triggers listener notifications.

**Minimal Example:** `{obj}.clearAllSequences();`

**Description:**
Removes all loaded MIDI sequences and tracks from this player. Sends an async notification to all sequence listeners (triggering the sequence callback if set).

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.setFile$`
- `$API.MidiPlayer.create$`

---

## connectToMetronome

**Signature:** `undefined connectToMetronome(String metronome)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Processor lookup traverses the module tree.
**Minimal Example:** `{obj}.connectToMetronome("Metronome1");`

**Description:**
Connects this MIDI player to a MidiMetronome effect module by its processor ID. Once connected, the metronome will follow the player's transport state and position.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| metronome | String | no | The processor ID of a MidiMetronome module in the signal chain. | Must be a valid MidiMetronome processor ID. |

**Pitfalls:**
- If the given ID does not match a MidiMetronome processor, a script error is thrown. No partial matching or type-agnostic lookup is performed.

---

## connectToPanel

**Signature:** `undefined connectToPanel(ScriptObject panel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Stores a weak reference to the panel object.
**Minimal Example:** `{obj}.connectToPanel(Panel1);`

**Description:**
Connects this MIDI player to a ScriptPanel for automatic UI updates. The connected panel receives `repaint()` calls when the sequence changes (always) and when the playback position changes (if `setRepaintOnPositionChange(true)` is also called).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| panel | ScriptObject | no | A ScriptPanel component reference. | Must be a ScriptPanel instance. |

**Pitfalls:**
- Passing a non-ScriptPanel object throws a script error ("Invalid panel").

**Cross References:**
- `$API.MidiPlayer.setRepaintOnPositionChange$`

---

## convertEventListToNoteRectangles

**Signature:** `Array convertEventListToNoteRectangles(Array eventList, Array targetBounds)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates a temporary HiseMidiSequence and performs heap allocations for rectangle construction.
**Minimal Example:** `var rects = {obj}.convertEventListToNoteRectangles(events, [0, 0, 500, 200]);`

**Description:**
Converts an array of MessageHolder objects into an array of note rectangles scaled to the given target bounds. Unlike `getNoteRectangleList()` which reads from the current sequence, this method operates on an arbitrary event list -- useful for previewing edited event data before flushing it.

Each rectangle is `[x, y, width, height]` where x/width represent time position/duration normalised to the sequence length, and y/height represent note number (127 at top, 0 at bottom, each note 1/128 of the height).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventList | Array | no | Array of MessageHolder objects to convert. | Each element must be a MessageHolder. |
| targetBounds | Array | no | Target rectangle as `[x, y, width, height]` to scale the output to. | Must be a valid 4-element bounds array. |

**Cross References:**
- `$API.MidiPlayer.getNoteRectangleList$`
- `$API.MidiPlayer.getEventList$`

---

## create

**Signature:** `undefined create(Integer nominator, Integer denominator, Integer barLength)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new HiseMidiSequence and adds it to the sequence list.
**Minimal Example:** `{obj}.create(4, 4, 4);`

**Description:**
Creates a new empty MIDI sequence with the given time signature and bar count, adds it to the player's sequence list, and selects it as the current sequence. Does NOT clear existing sequences -- call `clearAllSequences()` first if you want to replace them.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| nominator | Integer | no | Numerator of the time signature (e.g. 4 for 4/4). | Must be > 0. |
| denominator | Integer | no | Denominator of the time signature (e.g. 4 for 4/4). | Must be > 0. |
| barLength | Integer | no | Number of bars in the sequence. | Must be > 0. |

**Cross References:**
- `$API.MidiPlayer.clearAllSequences$`
- `$API.MidiPlayer.setFile$`

**Example:**
```javascript:create-sequence-3-4
// Title: Create an 8-bar sequence in 3/4 time
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.clearAllSequences();
mp.create(3, 4, 8);
```
```json:testMetadata:create-sequence-3-4
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getNumSequences()", "value": 1},
    {"type": "REPL", "expression": "mp.getTimeSignature().Nominator", "value": 3},
    {"type": "REPL", "expression": "mp.getTimeSignature().NumBars", "value": 8}
  ]
}
```

---

## flushMessageList

**Signature:** `undefined flushMessageList(Array messageList)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates EditAction, writes to sequence. Undoable operation.
**Minimal Example:** `{obj}.flushMessageList(events);`

**Description:**
Writes the given array of MessageHolder objects into the current sequence, replacing the existing MIDI data on the current track. This operation is undoable if `setUseGlobalUndoManager(true)` was called. Internally delegates to `flushMessageListToSequence()` with the current sequence index.

Event timestamps are interpreted as samples (default) or ticks, depending on `setUseTimestampInTicks()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| messageList | Array | no | Array of MessageHolder objects containing the MIDI events to write. | Each element must be a MessageHolder. Non-MessageHolder items trigger a script error. |

**Cross References:**
- `$API.MidiPlayer.flushMessageListToSequence$`
- `$API.MidiPlayer.getEventList$`
- `$API.MidiPlayer.setUseTimestampInTicks$`
- `$API.MidiPlayer.undo$`

---

## flushMessageListToSequence

**Signature:** `undefined flushMessageListToSequence(Array messageList, Integer sequenceIndexOneBased)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates EditAction, writes to sequence. Undoable operation.
**Minimal Example:** `{obj}.flushMessageListToSequence(events, 1);`

**Description:**
Writes the given array of MessageHolder objects into the sequence at the specified one-based index, replacing the existing MIDI data on the current track. This operation is undoable if `setUseGlobalUndoManager(true)` was called.

Event timestamps are interpreted as samples (default) or ticks, depending on `setUseTimestampInTicks()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| messageList | Array | no | Array of MessageHolder objects to write. | Each element must be a MessageHolder. |
| sequenceIndexOneBased | Integer | no | One-based index of the target sequence. | Must be >= 1. Invalid index triggers a script error. |

**Cross References:**
- `$API.MidiPlayer.flushMessageList$`
- `$API.MidiPlayer.getEventListFromSequence$`
- `$API.MidiPlayer.setUseTimestampInTicks$`

---

## getEventList

**Signature:** `Array getEventList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates MessageHolder objects on the heap for each event.
**Minimal Example:** `var events = {obj}.getEventList();`

**Description:**
Returns an array of MessageHolder objects representing all MIDI events in the current sequence's current track. Each MessageHolder wraps a HiseEvent with note number, velocity, channel, and timestamp. Timestamps use samples (default) or ticks depending on `setUseTimestampInTicks()`.

Internally delegates to `getEventListFromSequence(-1)` where -1 means "current sequence."

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.getEventListFromSequence$`
- `$API.MidiPlayer.flushMessageList$`
- `$API.MidiPlayer.setUseTimestampInTicks$`

**Example:**
```javascript:transpose-events-up-octave
// Title: Read events, transpose all notes up by 12, and write back
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);
mp.create(4, 4, 1);

// Populate with a test note
var noteList = [];
var on = Engine.createMessageHolder();
var off = Engine.createMessageHolder();
on.setType(on.NoteOn);
off.setType(on.NoteOff);
on.setNoteNumber(60);
off.setNoteNumber(60);
on.setVelocity(100);
on.setTimestamp(0);
off.setTimestamp(960);
on.setChannel(1);
off.setChannel(1);
noteList.push(on);
noteList.push(off);
mp.flushMessageList(noteList);

// Now transpose
var events = mp.getEventList();

for (e in events)
{
    if (e.isNoteOn() || e.isNoteOff())
        e.setNoteNumber(e.getNoteNumber() + 12);
}

mp.flushMessageList(events);
```
```json:testMetadata:transpose-events-up-octave
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mp.getEventList()[0].getNoteNumber()", "value": 72}
  ]
}
```

---

## getEventListFromSequence

**Signature:** `Array getEventListFromSequence(Integer sequenceIndexOneBased)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates MessageHolder objects on the heap for each event.
**Minimal Example:** `var events = {obj}.getEventListFromSequence(1);`

**Description:**
Returns an array of MessageHolder objects for all MIDI events in the specified sequence's current track. Timestamps use samples (default) or ticks depending on `setUseTimestampInTicks()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sequenceIndexOneBased | Integer | no | One-based index of the sequence to read. | Must be >= 1. Index 0 triggers a script error ("Nope. One based!!!"). |

**Cross References:**
- `$API.MidiPlayer.getEventList$`
- `$API.MidiPlayer.flushMessageListToSequence$`

---

## getLastPlayedNotePosition

**Signature:** `Double getLastPlayedNotePosition()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a cached value from the sequence.
**Minimal Example:** `var pos = {obj}.getLastPlayedNotePosition();`

**Description:**
Returns the normalised position (0.0-1.0) of the last note that was played during playback. Returns -1 if the player is stopped.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.getPlaybackPosition$`

---

## getMidiFileList

**Signature:** `Array getMidiFileList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates the MIDI file pool and constructs String objects.
**Minimal Example:** `var files = {obj}.getMidiFileList();`

**Description:**
Returns an array of string references for all MIDI files embedded in the plugin's MIDI file pool. These reference strings can be passed to `setFile()` to load a specific MIDI file.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.setFile$`

---

## getNoteRectangleList

**Signature:** `Array getNoteRectangleList(Array targetBounds)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates sequence events and constructs rectangle objects on the heap.
**Minimal Example:** `var rects = {obj}.getNoteRectangleList([0, 0, 500, 200]);`

**Description:**
Returns an array of rectangles representing all notes in the current sequence, scaled to the given target bounds. Each rectangle is `[x, y, width, height]` where x/width are time position/duration and y/height are note number (127 at top, each note 1/128 of height). Useful for drawing a piano roll visualization in a ScriptPanel paint routine.

Returns an empty array if no sequence is loaded.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetBounds | Array | no | Target rectangle as `[x, y, width, height]` to scale the note positions to. | Must be a valid 4-element bounds array. |

**Cross References:**
- `$API.MidiPlayer.convertEventListToNoteRectangles$`
- `$API.MidiPlayer.connectToPanel$`

---

## getNumSequences

**Signature:** `Integer getNumSequences()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumSequences();`

**Description:**
Returns the number of MIDI sequences currently loaded in this player. Returns 0 if no player reference exists.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.setSequence$`
- `$API.MidiPlayer.setFile$`

---

## getNumTracks

**Signature:** `Integer getNumTracks()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumTracks();`

**Description:**
Returns the number of tracks in the current sequence. Returns 0 if no sequence is loaded or no player reference exists.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.setTrack$`

---

## getPlaybackPosition

**Signature:** `Double getPlaybackPosition()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads a cached double value.
**Minimal Example:** `var pos = {obj}.getPlaybackPosition();`

**Description:**
Returns the current playback position as a normalised value between 0.0 and 1.0 within the current loop range. Returns 0.0 if no sequence is loaded.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.setPlaybackPosition$`
- `$API.MidiPlayer.getLastPlayedNotePosition$`

---

## getPlayState

**Signature:** `Integer getPlayState()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var state = {obj}.getPlayState();`

**Description:**
Returns the current transport state as an integer: 0 = Stop, 1 = Play, 2 = Record.

**Parameters:**

(None.)

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| 0 | Player is stopped, no playback or recording active. |
| 1 | Player is actively playing MIDI events from the sequence. |
| 2 | Player is recording incoming MIDI events into the sequence. |

**Cross References:**
- `$API.MidiPlayer.play$`
- `$API.MidiPlayer.stop$`
- `$API.MidiPlayer.record$`
- `$API.MidiPlayer.setPlaybackCallback$`

---

## getTicksPerQuarter

**Signature:** `Integer getTicksPerQuarter()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Returns a compile-time constant (960).
**Minimal Example:** `var tpq = {obj}.getTicksPerQuarter();`

**Description:**
Returns the MIDI tick resolution per quarter note. Always returns 960. Use this to convert between tick timestamps and musical positions when `setUseTimestampInTicks(true)` is active.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.setUseTimestampInTicks$`

---

## getTimeSignature

**Signature:** `JSON getTimeSignature()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a DynamicObject on the heap.
**Minimal Example:** `var sig = {obj}.getTimeSignature();`

**Description:**
Returns the time signature of the current sequence as a JSON object. Internally delegates to `getTimeSignatureFromSequence(-1)`.

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Nominator | Double | Numerator of the time signature (e.g. 4 for 4/4). |
| Denominator | Double | Denominator of the time signature (e.g. 4 for 4/4). |
| NumBars | Double | Number of bars in the sequence. |
| LoopStart | Double | Normalised loop start position (0.0-1.0). |
| LoopEnd | Double | Normalised loop end position (0.0-1.0). |
| Tempo | Double | BPM value (read-only -- not consumed by setTimeSignature). |

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.getTimeSignatureFromSequence$`
- `$API.MidiPlayer.setTimeSignature$`

---

## getTimeSignatureFromSequence

**Signature:** `JSON getTimeSignatureFromSequence(Integer index)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a DynamicObject on the heap.
**Minimal Example:** `var sig = {obj}.getTimeSignatureFromSequence(1);`

**Description:**
Returns the time signature of the sequence at the given one-based index as a JSON object. Returns an empty var if the sequence doesn't exist. See `getTimeSignature()` for the object property format.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | One-based index of the sequence. -1 returns the current sequence. | One-based, or -1 for current. |

**Cross References:**
- `$API.MidiPlayer.getTimeSignature$`
- `$API.MidiPlayer.setTimeSignatureToSequence$`

---

## isEmpty

**Signature:** `Integer isEmpty()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var empty = {obj}.isEmpty();`

**Description:**
Returns true if the player has no valid sequence loaded (either the player reference is null or no current sequence exists). This checks for any sequence presence, not whether the sequence contains MIDI data -- use `isSequenceEmpty()` for that.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.isSequenceEmpty$`

---

## isSequenceEmpty

**Signature:** `Integer isSequenceEmpty(Integer indexOneBased)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var empty = {obj}.isSequenceEmpty(1);`

**Description:**
Returns true if the sequence at the given one-based index contains no MIDI events. Returns true if the sequence doesn't exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| indexOneBased | Integer | no | One-based index of the sequence to check. | Must be >= 1. |

**Cross References:**
- `$API.MidiPlayer.isEmpty$`
- `$API.MidiPlayer.getNumSequences$`

---

## play

**Signature:** `Integer play(Integer timestamp)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Delegates to MidiPlayer::play() which modifies transport state and calls sendPlaybackChangeMessage(). Safe for audio thread calling but triggers listener notifications. When syncToMasterClock is true, this is a no-op (returns false).
**Minimal Example:** `{obj}.play(0);`

**Description:**
Starts playback. Pass 0 for immediate start, or pass a sample-accurate timestamp to delay the start within the current audio buffer. Returns true if playback started, false if the player couldn't start (e.g. synced to master clock).

When `syncToMasterClock` is enabled, this method returns false without doing anything -- transport is controlled by the master clock.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestamp | Integer | no | Sample offset within the current buffer for sample-accurate start. | 0 for immediate, or use `Message.getTimestamp()` for sample-accurate timing. |

**Cross References:**
- `$API.MidiPlayer.stop$`
- `$API.MidiPlayer.record$`
- `$API.MidiPlayer.getPlayState$`
- `$API.MidiPlayer.setSyncToMasterClock$`

---

## record

**Signature:** `Integer record(Integer timestamp)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Delegates to MidiPlayer::record() which modifies transport state. When syncToMasterClock is true and stopped, defers via recordOnNextPlaybackStart flag (returns false).
**Minimal Example:** `{obj}.record(0);`

**Description:**
Starts recording incoming MIDI events into the current sequence. Pass 0 for immediate start. Returns true if recording started, false if deferred (e.g. synced to master clock and stopped -- recording will start when the clock starts).

The recording uses overdub mode by default, where new notes are merged with existing sequence data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestamp | Integer | no | Sample offset for sample-accurate start. | 0 for immediate. |

**Cross References:**
- `$API.MidiPlayer.play$`
- `$API.MidiPlayer.stop$`
- `$API.MidiPlayer.setRecordEventCallback$`

---

## redo

**Signature:** `undefined redo()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls UndoManager::redo() which replays actions on the heap.
**Minimal Example:** `{obj}.redo();`

**Description:**
Redoes the last undone edit operation. Throws a script error if undo is deactivated (call `setUseGlobalUndoManager(true)` first). Does nothing if no sequence is loaded.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.undo$`
- `$API.MidiPlayer.setUseGlobalUndoManager$`

---

## reset

**Signature:** `undefined reset()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Resets sequence data, involves pool operations.
**Minimal Example:** `{obj}.reset();`

**Description:**
Resets the current sequence to the state it was in when last loaded from a MIDI file. Discards any edits made via `flushMessageList()` or recording. Does nothing if no sequence is loaded.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.undo$`
- `$API.MidiPlayer.setFile$`

---

## saveAsMidiFile

**Signature:** `Integer saveAsMidiFile(String file, Integer trackIndex)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O operation, writes to disk and reloads pool.
**Minimal Example:** `{obj}.saveAsMidiFile("{PROJECT_FOLDER}output.mid", 0);`

**Description:**
Saves the current sequence to a MIDI file at the given path, writing the data to the specified track index within the file. The track index determines which track in the output MIDI file receives the data. Returns true on success.

After saving, the MIDI file pool is reloaded so the file is immediately available via `getMidiFileList()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| file | String | no | File path or reference string for the output MIDI file. | Must be a valid file path. Empty string triggers a script error. |
| trackIndex | Integer | no | Track index in the output MIDI file (zero-based). | >= 0. |

**Cross References:**
- `$API.MidiPlayer.setFile$`
- `$API.MidiPlayer.getMidiFileList$`

---

## setAutomationHandlerConsumesControllerEvents

**Signature:** `undefined setAutomationHandlerConsumesControllerEvents(Integer shouldBeEnabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setAutomationHandlerConsumesControllerEvents(true);`

**Description:**
When enabled, CC messages played back from the MIDI file are sent to the global MIDI automation handler. The automation handler can consume these events (preventing them from passing through the signal chain), effectively allowing MIDI file playback to drive automated parameters.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeEnabled | Integer | no | Whether to enable CC routing to the automation handler. | Boolean (0 or 1). |

---

## setFile

**Signature:** `Integer setFile(String fileName, Integer clearExistingSequences, Integer selectNewSequence)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Loads from pool, allocates sequence objects, modifies sequence list.
**Minimal Example:** `{obj}.setFile("{PROJECT_FOLDER}myfile.mid", true, true);`

**Description:**
Loads a MIDI file from the pool and optionally clears existing sequences and selects the newly loaded one. Returns true if the file reference is valid (or if an empty filename was passed, which is a successful no-op).

The filename should be a pool reference string (use `getMidiFileList()` to get valid references) or a file path using `{PROJECT_FOLDER}` syntax.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Pool reference string or file path for the MIDI file. | Empty string is valid (no-op). |
| clearExistingSequences | Integer | no | If true, removes all existing sequences before loading. | Boolean (0 or 1). |
| selectNewSequence | Integer | no | If true, selects the newly loaded sequence as current. | Boolean (0 or 1). |

**Cross References:**
- `$API.MidiPlayer.getMidiFileList$`
- `$API.MidiPlayer.clearAllSequences$`
- `$API.MidiPlayer.create$`

**Example:**
```javascript:load-midi-from-pool
// Title: Load a MIDI file, replacing all existing sequences
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var files = mp.getMidiFileList();

if (files.length > 0)
    mp.setFile(files[0], true, true);
```
```json:testMetadata:load-midi-from-pool
{
  "testable": false,
  "skipReason": "Requires MIDI files in the project pool which cannot be created programmatically"
}
```

---

## setGlobalPlaybackRatio

**Signature:** `undefined setGlobalPlaybackRatio(Double globalRatio)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets an atomic value on MainController.
**Minimal Example:** `{obj}.setGlobalPlaybackRatio(0.5);`

**Description:**
Sets a global playback speed multiplier that affects ALL MidiPlayer instances. The effective playback speed of each player is `playerSpeed * globalRatio`. Use this for global tempo scaling.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| globalRatio | Double | no | Global speed multiplier. 1.0 = normal speed, 0.5 = half speed, 2.0 = double speed. | Must be > 0. |

**Pitfalls:**
- This affects ALL MidiPlayer instances globally, not just the one you call it on. The setting lives on the MainController.

---

## setPlaybackCallback

**Signature:** `undefined setPlaybackCallback(Function playbackCallback, Number synchronous)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a PlaybackUpdater object, registers as PlaybackListener.
**Minimal Example:** `{obj}.setPlaybackCallback(onPlaybackChange, 0);`

**Description:**
Registers a callback that fires whenever the transport state changes (play, stop, record). The callback receives two arguments: the timestamp and the new play state (0/1/2).

The `synchronous` parameter controls threading: when 0 (async), the callback fires on the UI thread via a deferred timer. When non-zero (sync), the callback fires directly on the audio thread -- the callback must be an inline function for realtime safety.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| playbackCallback | Function | yes | Callback function with two parameters. | Must be a JavaScript function. |
| synchronous | Number | yes | 0 for async (UI thread), non-zero for synchronous (audio thread). | Boolean-like: 0 = async, 1/true = sync. |

**Callback Signature:** playbackCallback(timestamp: int, playState: int)

**Example:**
```javascript:async-playback-state-callback
// Title: Async playback callback for transport state logging
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.create(4, 4, 1);

reg lastPlayState = -1;

// Async callback (safe for UI operations)
inline function onPlaybackChange(timestamp, playState)
{
    lastPlayState = playState;
}

mp.setPlaybackCallback(onPlaybackChange, 0);

// --- test-only ---
mp.play(0);
// --- end test-only ---
```
```json:testMetadata:async-playback-state-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "lastPlayState", "value": 1}
  ]
}
```

**Pitfalls:**
- When using synchronous mode, the callback runs on the audio thread. In the HISE IDE (USE_BACKEND), a realtime safety check validates that the callback is safe. In exported plugins, only `isRealtimeSafe()` is checked.

**Cross References:**
- `$API.MidiPlayer.getPlayState$`
- `$API.MidiPlayer.play$`
- `$API.MidiPlayer.stop$`
- `$API.MidiPlayer.record$`

---

## setPlaybackPosition

**Signature:** `undefined setPlaybackPosition(Double newPosition)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls setAttribute with sendNotificationAsync, which involves attribute change notification.
**Minimal Example:** `{obj}.setPlaybackPosition(0.5);`

**Description:**
Sets the playback position within the current loop. The value is clamped to 0.0-1.0. Does nothing if no sequence is loaded.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newPosition | Double | no | Normalised position between 0.0 (start) and 1.0 (end). | Clamped to [0.0, 1.0]. |

**Cross References:**
- `$API.MidiPlayer.getPlaybackPosition$`

---

## setRecordEventCallback

**Signature:** `undefined setRecordEventCallback(Function recordEventCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates ScriptEventRecordProcessor, registers with player.
**Minimal Example:** `{obj}.setRecordEventCallback(onRecordEvent);`

**Description:**
Registers an inline function that processes every MIDI event about to be recorded. The callback receives a single MessageHolder argument and runs on the audio thread. You can modify the event (e.g. quantize timestamps, filter notes) -- changes are applied to the recorded data.

The callback MUST be an inline function (realtime-safe). Regular functions will throw a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| recordEventCallback | Function | yes | Inline function with one MessageHolder parameter. | Must be an inline function (realtime-safe). Non-callable objects throw a script error. |

**Callback Signature:** recordEventCallback(event: MessageHolder)

**Example:**
```javascript:quantize-recording-16th
// Title: Quantize recorded notes to 16th notes
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---
const var mp = Synth.getMidiPlayer("MidiPlayer1");

// Quantize recorded notes to 16th notes
inline function onRecordEvent(event)
{
    if (event.isNoteOn() || event.isNoteOff())
    {
        local tpq = mp.getTicksPerQuarter();
        local grid = tpq / 4; // 16th note grid
        local ts = event.getTimestamp();
        event.setTimestamp(Math.round(ts / grid) * grid);
    }
}

mp.setUseTimestampInTicks(true);
mp.setRecordEventCallback(onRecordEvent);
```
```json:testMetadata:quantize-recording-16th
{
  "testable": false,
  "skipReason": "Record event callback only fires during active recording with incoming MIDI events on the audio thread"
}
```

**Cross References:**
- `$API.MidiPlayer.record$`

---

## setRepaintOnPositionChange

**Signature:** `undefined setRepaintOnPositionChange(Integer shouldRepaintPanel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Starts/stops SuspendableTimer (message thread timer).
**Minimal Example:** `{obj}.setRepaintOnPositionChange(true);`

**Description:**
When enabled, the connected panel (set via `connectToPanel()`) receives `repaint()` calls at 50ms intervals whenever the playback position changes. When disabled, the panel is only repainted when the sequence data changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldRepaintPanel | Integer | no | Whether to enable position-change repainting. | Boolean (0 or 1). |

**Cross References:**
- `$API.MidiPlayer.connectToPanel$`

---

## setSequence

**Signature:** `undefined setSequence(Integer sequenceIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls setAttribute with sendNotificationAsync.
**Minimal Example:** `{obj}.setSequence(1);`

**Description:**
Selects one of the previously loaded sequences as the current active sequence. Uses one-based indexing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sequenceIndex | Integer | no | One-based index of the sequence to activate. | Must be >= 1. |

**Cross References:**
- `$API.MidiPlayer.getNumSequences$`
- `$API.MidiPlayer.setFile$`

---

## setSequenceCallback

**Signature:** `undefined setSequenceCallback(Function updateFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates WeakCallbackHolder, stores callback reference.
**Minimal Example:** `{obj}.setSequenceCallback(onSequenceChange);`

**Description:**
Registers a callback that fires whenever the MIDI sequence data changes (load, edit, clear). The callback receives one argument: a reference to this MidiPlayer object. The callback fires asynchronously on the message thread.

Immediately fires the callback once upon registration (so you can initialise your UI state).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| updateFunction | Function | yes | Callback function with one parameter. | Must be a JavaScript function. |

**Callback Signature:** updateFunction(midiPlayer: MidiPlayer)

**Example:**
```javascript:sequence-change-repaint
// Title: Sequence change callback triggering panel repaint
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---
const var mp = Synth.getMidiPlayer("MidiPlayer1");
const var Panel1 = Content.addPanel("Panel1", 0, 0);

reg callbackFired = false;

inline function onSequenceChange(player)
{
    callbackFired = true;
    Panel1.repaint();
}

mp.setSequenceCallback(onSequenceChange);
// The callback fires immediately on registration
```
```json:testMetadata:sequence-change-repaint
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "callbackFired", "value": true}
  ]
}
```

**Cross References:**
- `$API.MidiPlayer.setPlaybackCallback$`
- `$API.MidiPlayer.connectToPanel$`

---

## setSyncToMasterClock

**Signature:** `undefined setSyncToMasterClock(Integer shouldSyncToMasterClock)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies player state and validates master clock availability.
**Minimal Example:** `{obj}.setSyncToMasterClock(true);`

**Description:**
Enables or disables synchronisation of this MIDI player's transport to the master clock (host transport or internal clock). When enabled, manual `play()` and `stop()` calls become no-ops -- transport is driven by the master clock.

Throws a script error if the master clock grid is not enabled when trying to enable sync.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldSyncToMasterClock | Integer | no | Whether to sync to master clock. | Boolean (0 or 1). |

**Pitfalls:**
- You must enable the master clock grid before calling this with `true`. Otherwise a script error is thrown.
- When synced, `play()` and `stop()` return false without doing anything. Only `record()` has a special handling (defers to next clock start).

**Cross References:**
- `$API.MidiPlayer.play$`
- `$API.MidiPlayer.stop$`
- `$API.MidiPlayer.record$`
- `$API.TransportHandler.setOnGridChange$`

---

## setTimeSignature

**Signature:** `Integer setTimeSignature(JSON timeSignatureObject)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Modifies sequence length, may use undo manager.
**Minimal Example:** `{obj}.setTimeSignature({"Nominator": 3, "Denominator": 4, "NumBars": 8});`

**Description:**
Sets the time signature and length of the current sequence using a JSON object. Returns true if the values are valid. Internally delegates to `setTimeSignatureToSequence(-1, ...)`.

The Tempo property from `getTimeSignature()` is NOT consumed -- only Nominator, Denominator, NumBars, LoopStart, and LoopEnd are read.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timeSignatureObject | JSON | no | Object with time signature properties. | Must have Nominator > 0, Denominator > 0, NumBars > 0. |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Nominator | Double | Numerator of the time signature (required). |
| Denominator | Double | Denominator of the time signature (required). |
| NumBars | Double | Number of bars (required). |
| LoopStart | Double | Normalised loop start (0.0-1.0, defaults to 0.0). |
| LoopEnd | Double | Normalised loop end (0.0-1.0, defaults to 1.0). |

**Pitfalls:**
- The Tempo property from `getTimeSignature()` is ignored when setting. Tempo is derived from the host/master clock, not the sequence metadata.

**Cross References:**
- `$API.MidiPlayer.getTimeSignature$`
- `$API.MidiPlayer.setTimeSignatureToSequence$`

---

## setTimeSignatureToSequence

**Signature:** `Integer setTimeSignatureToSequence(Integer index, JSON timeSignatureObject)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Modifies sequence length, may use undo manager.
**Minimal Example:** `{obj}.setTimeSignatureToSequence(1, {"Nominator": 4, "Denominator": 4, "NumBars": 4});`

**Description:**
Sets the time signature and length of the sequence at the given one-based index. Returns true if the values are valid, false if the sequence doesn't exist or values are invalid (any of Nominator, Denominator, NumBars <= 0).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | One-based sequence index, or -1 for current sequence. | One-based, or -1. |
| timeSignatureObject | JSON | no | Object with time signature properties. | See setTimeSignature for property format. |

**Cross References:**
- `$API.MidiPlayer.setTimeSignature$`
- `$API.MidiPlayer.getTimeSignatureFromSequence$`

---

## setTrack

**Signature:** `undefined setTrack(Integer trackIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls setAttribute with sendNotificationAsync.
**Minimal Example:** `{obj}.setTrack(1);`

**Description:**
Sets the active track within the current sequence. Uses one-based indexing. Affects which track is read by `getEventList()` and written to by `flushMessageList()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| trackIndex | Integer | no | One-based track index. | Must be >= 1. |

**Cross References:**
- `$API.MidiPlayer.getNumTracks$`

---

## setUseGlobalUndoManager

**Signature:** `undefined setUseGlobalUndoManager(Integer shouldUseGlobalUndoManager)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setUseGlobalUndoManager(true);`

**Description:**
Enables or disables undo tracking for MIDI edit operations. When enabled, uses the global undo manager shared with `Engine.undo()`. Undo is disabled by default.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseGlobalUndoManager | Integer | no | Whether to enable the global undo manager. | Boolean (0 or 1). |

**Pitfalls:**
- Undo is disabled by default. Calling `undo()` or `redo()` without first enabling the undo manager throws a script error ("Undo is deactivated").
- Calling `setUseGlobalUndoManager(false)` after previously enabling it destroys the internal undo manager. Undo cannot be re-enabled using the internal manager after this -- only the global manager can be re-attached by calling `setUseGlobalUndoManager(true)` again.

**Cross References:**
- `$API.MidiPlayer.undo$`
- `$API.MidiPlayer.redo$`
- `$API.MidiPlayer.flushMessageList$`

---

## setUseTimestampInTicks

**Signature:** `undefined setUseTimestampInTicks(Integer shouldUseTicksAsTimestamps)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setUseTimestampInTicks(true);`

**Description:**
Switches the timestamp format for event list operations between samples (default) and MIDI ticks. When enabled, `getEventList()`, `getEventListFromSequence()`, `flushMessageList()`, and `flushMessageListToSequence()` use tick timestamps (960 ticks per quarter note). When disabled, they use sample timestamps at the current sample rate and BPM.

Tick mode is recommended for musical editing (quantization, grid alignment) since tick values are tempo-independent.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseTicksAsTimestamps | Integer | no | Whether to use ticks (true) or samples (false) for timestamps. | Boolean (0 or 1). |

**Cross References:**
- `$API.MidiPlayer.getTicksPerQuarter$`
- `$API.MidiPlayer.getEventList$`
- `$API.MidiPlayer.flushMessageList$`

---

## stop

**Signature:** `Integer stop(Integer timestamp)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Delegates to MidiPlayer::stop() which modifies transport state and calls sendPlaybackChangeMessage(). When syncToMasterClock is true, this is a no-op (returns false).
**Minimal Example:** `{obj}.stop(0);`

**Description:**
Stops playback or recording. Pass 0 for immediate stop, or pass a timestamp for sample-accurate stopping. Returns true if the player was stopped, false if it couldn't be stopped (e.g. synced to master clock).

If the player was recording, stops recording and flushes recorded events to the sequence. Also sends note-off messages for any currently sounding notes and handles sustain pedal cleanup.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestamp | Integer | no | Sample offset for sample-accurate stop. | 0 for immediate. |

**Cross References:**
- `$API.MidiPlayer.play$`
- `$API.MidiPlayer.record$`
- `$API.MidiPlayer.setSyncToMasterClock$`

---

## undo

**Signature:** `undefined undo()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls UndoManager::undo() which replays actions on the heap.
**Minimal Example:** `{obj}.undo();`

**Description:**
Undoes the last MIDI edit operation (flushMessageList, setTimeSignature, etc.). Throws a script error if undo is deactivated. Does nothing if no sequence is loaded.

**Parameters:**

(None.)

**Cross References:**
- `$API.MidiPlayer.redo$`
- `$API.MidiPlayer.setUseGlobalUndoManager$`
- `$API.MidiPlayer.flushMessageList$`
