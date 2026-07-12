# MidiPlayer

MidiPlayer provides scripting control over a MIDI Player processor module in the signal chain. Obtain a reference with `Synth.getMidiPlayer("processorId")`. The player manages one or more MIDI sequences, each containing one or more tracks. Typical workflows include:

1. Loading MIDI files from the pool and switching between sequences at runtime.
2. Creating empty sequences programmatically and populating them via `flushMessageList()`.
3. Recording incoming MIDI into a sequence with optional real-time event filtering.
4. Extracting note rectangles for drawing a piano-roll overlay on a ScriptPanel.

Sequences and tracks use one-based indexing in the scripting API. Each player has its own undo manager by default, so `undo()` and `redo()` work without extra setup. Call `setUseGlobalUndoManager(true)` to switch to the global undo stack shared with `Engine.undo()`.

> [!Tip:Use tick timestamps for stable editing] Event timestamps default to sample counts at the current sample rate and BPM. Call `setUseTimestampInTicks(true)` to switch to tick timestamps (960 per quarter note), which are stable across tempo changes and recommended for musical editing. The timestamp mode affects both reading (`getEventList()`) and writing (`flushMessageList()`), so use the same mode for both.

## Common Mistakes

- **Disabling global undo destroys history**
  **Wrong:** Calling `setUseGlobalUndoManager(false)` after enabling it and expecting per-player undo to still work
  **Right:** Leave the default per-player undo alone, or commit to the global stack
  *Disabling the global undo manager destroys the per-player manager too. The previous undo history is lost.*

- **Sequence indices are 1-based**
  **Wrong:** `mp.setSequence(0);`
  **Right:** `mp.setSequence(1);`
  *Sequence and track indices are one-based. Index 0 triggers a script error.*

- **Tempo property is read-only**
  **Wrong:** Modifying `Tempo` in the time signature object and calling `setTimeSignature()`
  **Right:** Set tempo via the module attribute instead
  *`setTimeSignature()` does not consume the `Tempo` property from the JSON object - it is read-only.*

- **Use TransportHandler when clock-synced**
  **Wrong:** Calling `play()` / `stop()` when synced to master clock
  **Right:** Use `TransportHandler.startInternalClock()` / `TransportHandler.stopInternalClock()`
  *When `setSyncToMasterClock(true)` is active, `play()` and `stop()` are no-ops that return false. Transport must be driven through the TransportHandler.*

- **Clear sequences before creating new ones**
  **Wrong:** Creating sequences without clearing first
  **Right:** Call `clearAllSequences()` then `create()` in a loop
  *`create()` appends to the sequence list. Without clearing, repeated initialisation accumulates sequences.*

- **Use tick timestamps for stable editing**
  **Wrong:** Using sample timestamps for grid-aligned editing
  **Right:** Call `setUseTimestampInTicks(true)` before `getEventList()` / `flushMessageList()`
  *Sample timestamps shift with tempo changes. Tick timestamps (960 per quarter note) are stable musical positions.*
