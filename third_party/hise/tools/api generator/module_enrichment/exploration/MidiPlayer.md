# MidiPlayer - C++ Exploration

**Source:** `hi_core/hi_dsp/modules/MidiPlayer.h` (lines 248-727), `MidiPlayer.cpp` (lines 1071-2679)
**Base class:** `MidiProcessor`, also implements `TempoListener`

## Signal Path

MidiPlayer is a MIDI event generator, not a transformer. It injects artificial noteOn/noteOff/CC/pitchWheel events into the MIDI buffer from stored MIDI sequence data, synchronized to host tempo. Incoming MIDI events pass through unmodified (unless recording, in which case they are captured).

The playback engine runs in `preprocessBuffer()` (called before per-event processing), which is the main audio-thread callback. `processHiseEvent()` handles only recording of incoming live MIDI.

[playback position] -> getNextEvent(tick range) -> convert to HiseEvent -> inject into buffer -> advance position

## Gap Answers

### midi-event-generation

**Question:** How does processHiseEvent() / prepareToPlay() work? Does the MidiPlayer inject noteOn/noteOff events into the MIDI buffer based on the current position, or does it transform incoming events?

**Answer:** MidiPlayer is a pure MIDI generator. `preprocessBuffer()` (MidiPlayer.cpp:1429-1630) is the playback engine. It calculates a tick range for the current audio block based on `ticksPerSample * numSamples`, then calls `seq->getNextEvent(currentRange)` in a while loop to fetch all MIDI events that fall within that range. Each fetched event is converted to a HiseEvent, given a sample-accurate timestamp within the buffer, marked as artificial, assigned the current track's channel, and added to the buffer via `buffer.addEvent()`.

For noteOn events, it also fetches the matching noteOff via `seq->getMatchingNoteOffForCurrentEvent()`. If the noteOff falls within the current buffer, it is added immediately; otherwise it is deferred via `addHiseEventToBuffer()` (the MidiProcessor's future-event queue).

`processHiseEvent()` (MidiPlayer.cpp:1632-1725) does NOT generate playback events. It only handles recording: when in Record state, it captures incoming non-artificial MIDI events and stores them with tick timestamps into either `currentlyRecordedEvents` (simple recording) or `overdubNoteOns` (overdub mode).

`isProcessingWholeBuffer()` returns true, which is what enables `preprocessBuffer()` to be called.

### transport-control

**Question:** How is playback started and stopped? Is there a play/stop/record API, or does it respond to incoming MIDI? How does it sync to host transport?

**Answer:** Transport is controlled via three public methods: `play(timestamp)`, `stop(timestamp)`, `record(timestamp)` (MidiPlayer.cpp:2175-2206). These are called from the scripting API (MidiPlayer interface). Each accepts an optional sample-accurate timestamp for buffer-precise triggering.

There are two sync modes:
1. **Free-running (default):** `play()`/`stop()` directly call `startInternal()`/`stopInternal()` which set `playState`, reset position to 0, and notify listeners.
2. **Master clock sync:** When `syncToMasterClock` is true (set via `setSyncToMasterClock()`), `play()` and `stop()` are blocked (return false). Instead, transport responds to `onGridChange()` and `onTransportChange()` callbacks from the MainController's master clock. `onGridChange()` with `firstGridEventInPlayback=true` triggers `startInternal()` (or `recordInternal()` if `recordOnNextPlaybackStart` is set). `onTransportChange(false)` triggers `stopInternal()`.

All-notes-off messages also trigger `stop()` via `processHiseEvent()`.

### position-parameter-behavior

**Question:** Is CurrentPosition read-only or can it be set externally to seek? Does 'non-persistent' mean it is not saved with presets?

**Answer:** CurrentPosition is read-write. `getAttribute(CurrentPosition)` returns `getPlaybackPosition()` which is `fmod(currentPosition, 1.0)`. `setInternalAttribute(CurrentPosition)` (MidiPlayer.cpp:1315-1328) clamps the value to the loop region [loopStart, loopEnd], then converts the normalized position to ticks and sets `ticksSincePlaybackStart`, followed by `updatePositionInCurrentSequence()` which updates the sequence's playback index. So it can be used for seeking.

Non-persistent is confirmed: `exportAsValueTree()` (MidiPlayer.cpp:1169-1193) saves CurrentSequence, CurrentTrack, LoopEnabled, and PlaybackSpeed, but does NOT save CurrentPosition. It is purely runtime state.

### sequence-index-base

**Question:** CurrentSequence description says '1-based index' but min=0.0 and default=0.0. Is the index actually 0-based, or does 0 mean 'no sequence selected'?

**Answer:** The index IS 1-based. `getAttribute(CurrentSequence)` returns `(float)(currentSequenceIndex + 1)` (MidiPlayer.cpp:1296). `setInternalAttribute(CurrentSequence)` does `currentSequenceIndex = jlimit<int>(-1, ..., (int)(newAmount - 1))` (MidiPlayer.cpp:1355). So parameter value 1 = first sequence (internal index 0), value 2 = second sequence, etc. Value 0 maps to internal index -1, which means "no sequence selected" - `getCurrentSequence()` returns nullptr when `currentSequenceIndex == -1`.

The default of 0.0 means no sequence is selected initially. Sequences become selected when loaded via `addSequence(newSequence, select=true)` which sets the index to the last added sequence.

### track-selection-effect

**Question:** What does CurrentTrack actually control? Does it filter which MIDI track is played, or just affect the UI?

**Answer:** CurrentTrack affects BOTH playback and editing. `setInternalAttribute(CurrentTrack)` (MidiPlayer.cpp:1374-1384) sets `currentTrackIndex` and calls `seq->setCurrentTrackIndex(currentTrackIndex)`. In `preprocessBuffer()` (line 1483), `seq->setCurrentTrackIndex(currentTrackIndex)` is called before event fetching. The sequence's `getNextEvent()` and `getReadPointer()` methods use `currentTrackIndex` to select which track within a multi-track MIDI file to read events from. Generated events also have their channel set to `currentTrackIndex + 1` (line 1552).

So track selection determines which track of a multi-track MIDI file is played back AND which track is displayed/edited in overlays. It is 1-based (parameter value 1 = track 0 internally). Default is 1 (first track).

### loop-region-interaction

**Question:** When LoopEnabled is off, does the player still respect LoopStart/LoopEnd as a playback region?

**Answer:** When LoopEnabled is off, the player plays from the start and stops when `currentPosition > loopEnd` (MidiPlayer.cpp:1441). The check is: `if (currentPosition > loopEnd && (!loopEnabled || isRecording()))` - when loopEnabled is false, this condition is true at the loop end, and it calls `stop()` (line 1458). So LoopEnd acts as the endpoint even in one-shot mode.

However, LoopStart is only used for wrap-around in the loop-enabled path. In `getNextEvent()` (MidiPlayer.cpp:237-322), when `wrapAroundLoop` is detected (the current range contains `loopEndTicks`), the code splits into before-wrap and after-wrap ranges using `loopStartTicks` as the wrap target. When looping is disabled, the tick range is clamped to `lengthInTicks` (line 1509) instead of extending past the end.

In summary: one-shot mode plays from start to LoopEnd, then stops. LoopStart has no effect in one-shot mode.

### speed-tempo-relationship

**Question:** Does PlaybackSpeed interact with host BPM? Is MIDI file playback tempo-synced by default?

**Answer:** Yes, playback is tempo-synced to host BPM. `tempoChanged()` (MidiPlayer.cpp:1134-1137) is called whenever the host tempo changes and recalculates `ticksPerSample` based on the new BPM. The conversion uses `MidiPlayerHelpers::samplesToTicks(1, newTempo, getSampleRate())`. This means the MIDI file plays at the host's tempo, not at the MIDI file's embedded tempo.

PlaybackSpeed is a multiplier ON TOP of the tempo sync. `getTicksPerSample()` returns `ticksPerSample * getPlaybackSpeed()` (MidiPlayer.cpp:847). `getPlaybackSpeed()` returns `playbackSpeed * getMainController()->getGlobalPlaybackSpeed()` (MidiPlayer.cpp:850). So the effective playback rate = host BPM * PlaybackSpeed * globalPlaybackSpeed.

At PlaybackSpeed=1.0, a 4-bar MIDI file plays over exactly 4 bars at the host tempo. At PlaybackSpeed=2.0, it plays in half the time (double speed relative to host).

### midi-file-loading

**Question:** How are MIDI files loaded?

**Answer:** MIDI files are loaded via `loadMidiFile(PoolReference)` (MidiPlayer.cpp:1400-1421), which loads from the HISE MIDI file pool (supporting expansion packs). The loaded MidiFile is converted to a `HiseMidiSequence` and added to the internal sequence list. Multiple sequences can be loaded and selected via CurrentSequence.

The loading workflow:
1. **Scripting API:** The MidiPlayer scripting interface exposes methods to load MIDI files by reference string
2. **Drag-and-drop:** The `MidiFileDragAndDropper` overlay (registered via `ENABLE_OVERLAY_FACTORY`) provides drag-and-drop loading in the editor
3. **Preset restore:** Sequences are embedded in presets via `exportAsValueTree()` which serializes MIDI data as compressed base64-encoded MIDI file data

MIDI files are stored in the HISE pool system under `FileHandlerBase::MidiFiles`.

### overlay-integration

**Question:** How do the piano roll and step sequencer overlays interact with MidiPlayer?

**Answer:** MidiPlayer uses an overlay factory pattern. `MidiPlayerBaseType` (MidiPlayer.h:731-781) is the base class for overlays. Each overlay implements `MidiPlayer::SequenceListener` to receive sequence change notifications.

Registered overlays (via `ENABLE_OVERLAY_FACTORY` macro):
- **"Midi Viewer"** (`SimpleMidiViewer`): Read-only note display
- **"CC Viewer"** (`SimpleCCViewer`): CC event display (wraps SimpleMidiViewer)
- **"Looper"** (`MidiLooper`): Loop-based recording/editing overlay
- **"Drag 'n Drop"** (`MidiFileDragAndDropper`): File loading via drag-and-drop

These are hosted via the `MidiOverlayPanel` FloatingTile (SET_PANEL_NAME "MidiOverlayPanel"), which provides a ComboBox to select the overlay type. The overlay connects to any MidiPlayer processor in the module tree.

Overlays can edit sequences in real-time via `MidiPlayer::flushEdit()`, which applies an undoable `EditAction` that swaps the current track's MIDI data. The editing system supports full undo/redo.

## Processing Chain Detail

1. **NaN guard** (negligible): Reset `currentPosition` if NaN (MidiPlayer.cpp:1433-1434)
2. **Active check** (negligible): Skip if no sequence selected or position is -1 (stopped)
3. **Loop boundary check** (negligible): If position > loopEnd and not looping, stop playback or handle overdub wrap
4. **Stop check** (negligible): If playState is Stop, reset sequence and exit
5. **Position correction** (negligible): If position outside loop bounds, update sequence playback index
6. **Tick range calculation** (negligible): Calculate tick range for this buffer: `positionInTicks` to `positionInTicks + tickThisTime`
7. **Event fetch loop** (low): While loop calling `seq->getNextEvent(currentRange)`, limited to 16 events per block via duplicate detection array
8. **Event conversion and injection** (low): Convert each MidiMessage to HiseEvent, calculate sample-accurate timestamp, handle noteOn/noteOff/CC/pitchWheel, add to buffer
9. **Position advance** (negligible): `currentPosition += delta; ticksSincePlaybackStart += tickThisTime`

## Conditional Behavior

- **PlayState::Stop**: `preprocessBuffer()` resets sequence, sets position to -1, returns early
- **PlayState::Play**: Normal playback - generates events from sequence
- **PlayState::Record**: Playback continues AND `processHiseEvent()` captures incoming MIDI
- **LoopEnabled = true**: Position wraps from loopEnd back to loopStart; `getNextEvent()` handles wrap-around event fetching
- **LoopEnabled = false**: Playback stops when position exceeds loopEnd (one-shot)
- **syncToMasterClock = true**: `play()`/`stop()` are blocked; transport follows master clock grid events
- **overdubMode = true** (during recording): Note-on/off pairs are accumulated in `overdubNoteOns` and periodically flushed via `OverdubUpdater` timer
- **globalMidiHandlerConsumesCC = true**: CC events from the sequence are sent to the MIDI control automation handler instead of the buffer
- **noteOffAtStop = true**: When stopping, pending noteOff events are moved to the end of the current buffer
- **isBypassed()**: NoteOn events are not generated (CCs and pitchWheel still pass through)
- **Sustain pedal tracking**: CC64 events update `sustainPedalStates[]`; on stop, sustain-off events are sent for all active channels

## CPU Assessment

- **Overall baseline**: negligible to low
- **When playing**: low (event fetch is a sequential scan through pre-sorted MIDI events; at most 16 events per buffer block; timestamp conversion is simple arithmetic)
- **When stopped**: negligible (early return after position check)
- **Recording (overdub)**: low (additional event capture and periodic flush via timer)
- **No scaling factors**: CPU cost is roughly constant; limited to 16 events per block regardless of sequence density

## UI Components

- **Backend editor**: `MidiPlayerEditor` (MidiPlayerEditor.h:39-145) - transport buttons (play/stop/record), sequence selector ComboBox, track selector ComboBox, position slider, loop toggle, drag-and-drop zone, overlay type selector
- **FloatingTile**: `MidiOverlayPanel` (MidiOverlayFactory.h:120-189) - hosts overlay types with processor connection
- **Overlays**: SimpleMidiViewer ("Midi Viewer"), SimpleCCViewer ("CC Viewer"), MidiLooper ("Looper"), MidiFileDragAndDropper ("Drag 'n Drop")

## Notes

- The sequence `getNextEvent()` method (MidiPlayer.cpp:237-322) handles loop wrap-around by splitting the tick range into before-wrap and after-wrap sub-ranges, then scanning for events in both. It skips noteOff events when wrapping to avoid orphaned note-offs at the loop boundary.
- PlaybackSpeed interacts with `getMainController()->getGlobalPlaybackSpeed()`, so there is a global speed multiplier on top of the per-player speed.
- The internal tick resolution is 960 ticks per quarter note (`HiseMidiSequence::TicksPerQuarter = 960`).
- MIDI files are normalized on load: the file's native ticks-per-quarter is converted to the internal 960 TPQ, meta events are stripped, time signature and tempo are extracted, and empty tracks are discarded.
- The 16-event-per-block limit in `preprocessBuffer()` (the `eventsInThisCallback[16]` array) prevents runaway event generation from dense sequences but could silently drop events if more than 16 fall in a single buffer.
- LoopStart and LoopEnd are stored inside the sequence's TimeSignature object, not directly on the processor. They persist through the sequence's ValueTree serialization.
