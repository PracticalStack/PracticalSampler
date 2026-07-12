# MidiPlayer -- Project Context

## Project Context

### Real-World Use Cases
- **Step sequencer engine**: A drum machine uses one MidiPlayer per channel (e.g. 12 channels), each holding multiple pattern bank sequences created with `create()`. SliderPack data drives programmatic note generation via `flushMessageListToSequence()`, with tick-mode timestamps for grid-aligned editing. The MidiPlayers sync to a master clock and connect to a shared metronome. This is the most complex and feature-complete MidiPlayer use case.
- **MIDI file batch processor**: A sample-based instrument uses MidiPlayer as a utility for loading, transforming, and re-saving MIDI files - reading events with `getEventList()`, remapping note numbers and velocities, then saving with `saveAsMidiFile()`. The player is connected to a panel for visual feedback via `connectToPanel()` and `getNoteRectangleList()`. No playback transport is used - MidiPlayer acts purely as a MIDI file editor.
- **Multi-stem loop player**: Multiple MidiPlayer instances (e.g. 8) drive parallel samplers for loop playback. Each player loads a MIDI file per stem, and transport is managed collectively. MIDI files are loaded from the embedded pool using `setFile()`.

### Complexity Tiers
1. **MIDI file utility** (simplest): `setFile()`, `getEventList()`, `flushMessageList()`, `saveAsMidiFile()`, `connectToPanel()`, `getNoteRectangleList()`. Load, edit, and visualize MIDI data without transport.
2. **Playback engine**: Adds `play()`, `stop()`, `getPlaybackPosition()`, `setPlaybackCallback()`, `setRepaintOnPositionChange()`. Drive real-time MIDI playback with transport controls and position-synced UI.
3. **Sequencer with recording**: Adds `record()`, `setRecordEventCallback()`, `setSyncToMasterClock()`, `connectToMetronome()`, `setUseGlobalUndoManager()`. Full DAW-style sequencer with quantized recording, master clock sync, and undo support.
4. **Multi-pattern programmatic sequencer** (most complex): Adds `create()`, `flushMessageListToSequence()`, `setSequence()`, `setTimeSignature()`, `setUseTimestampInTicks()`, `getTicksPerQuarter()`, `isSequenceEmpty()`, `asMidiProcessor()`, `setGlobalPlaybackRatio()`, `setAutomationHandlerConsumesControllerEvents()`, `convertEventListToNoteRectangles()`. Multiple sequences per player, programmatic note construction from UI data, CC automation embedding, per-player speed control via MidiProcessor attributes.

### Practical Defaults
- Use `setUseTimestampInTicks(true)` for any musical editing work. Tick timestamps (960 per quarter note) are tempo-independent and align naturally to musical grid positions. Sample timestamps are only useful for audio-aligned processing.
- Call `clearAllSequences()` before `create()` when initializing a fresh player. `create()` appends to the sequence list rather than replacing.
- Initialize multiple pattern banks in a loop: call `create(4, 4, 2)` N times after `clearAllSequences()`, then `setSequence(1)` to select the first.
- Use `setFile(file, true, true)` (clear + select) as the standard pattern for loading a single MIDI file. Only pass `false` for the clear parameter when intentionally accumulating multiple sequences.
- When building a step sequencer, set `setAutomationHandlerConsumesControllerEvents(true)` so that CC messages embedded in the MIDI data can drive parameter automation during playback.

### Integration Patterns
- `MidiPlayer.setSyncToMasterClock(true)` requires `TransportHandler.setEnableGrid(true, gridIndex)` and `TransportHandler.setSyncMode()` to be configured first. Without the grid enabled, `setSyncToMasterClock()` throws a script error.
- `MidiPlayer.asMidiProcessor()` -> `MidiProcessor.getAttribute(6)` reads the PlaybackSpeed attribute (index 6), which can be used to calculate effective loop length and ruler spacing.
- `MidiPlayer.connectToPanel()` + `MidiPlayer.setRepaintOnPositionChange(true)` -> `ScriptPanel.setPaintRoutine()` with `getNoteRectangleList()` creates an auto-updating piano roll visualization.
- `MidiPlayer.setPlaybackCallback(fn, false)` fires asynchronously and is used to update UI state (button labels, recording indicators) when transport changes.
- `MidiPlayer.setRecordEventCallback(fn)` -> `MessageHolder.setTimestamp()` with quantization math using `getTicksPerQuarter()` for grid-aligned recording.
- `MidiPlayer.setGlobalPlaybackRatio(ratio)` is typically driven by a broadcaster to update all player instances simultaneously when a global speed control changes.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `play()` / `stop()` when synced to master clock | Use `TransportHandler.startInternalClock()` / `TransportHandler.stopInternalClock()` | When `setSyncToMasterClock(true)` is active, `play()` and `stop()` are no-ops that return false. Transport must be driven through the TransportHandler. |
| Creating sequences without clearing first | Call `clearAllSequences()` then `create()` in a loop | `create()` appends to the sequence list. Without clearing, repeated initialization accumulates sequences. |
| Using sample timestamps for grid-aligned editing | Call `setUseTimestampInTicks(true)` before `getEventList()` / `flushMessageList()` | Sample timestamps shift with tempo changes. Tick timestamps (960 per quarter note) are stable musical positions. |
