## startInternalClock

**Examples:**

```javascript:midi-triggered-clock-start-with
// Title: MIDI-triggered clock start with sample-accurate timestamp
// Context: A drum machine starts its sequencer when a specific MIDI note is received.
// Using Message.getTimestamp() ensures the clock starts at the exact sample position
// of the note event within the audio block, not at the block boundary.
const var th = Engine.createTransportHandler();
th.setSyncMode(th.PreferInternal);
th.setEnableGrid(true, 8); // 1/8 note grid

// In onNoteOn callback:
// Start clock at the note's sample position for tight timing
// th.startInternalClock(Message.getTimestamp());

// From a UI play button (no MIDI context, so timestamp 0 is fine):
// th.startInternalClock(0);

```
```json:testMetadata:midi-triggered-clock-start-with
{
  "testable": false
}
```


```javascript:auto-play-after-preset-load
// Title: Auto-play after preset load with grid resync
// Context: When loading a new preset while playback was active, the clock must be
// stopped before load and restarted after. sendGridSyncOnNextCallback() ensures the
// grid restarts cleanly instead of continuing from a stale position.
// --- setup ---
const var th0 = Engine.createTransportHandler();
th0.setSyncMode(th0.InternalOnly);
th0.stopInternalClock(0);
Engine.setHostBpm(120);
// --- end setup ---
const var th = Engine.createTransportHandler();
th.setEnableGrid(true, 8);

// Before preset load: stop the clock
th.stopInternalClock(0);

// After preset load completes: resync and restart
th.sendGridSyncOnNextCallback();
th.startInternalClock(0);
```
```json:testMetadata:auto-play-after-preset-load
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "th.isPlaying()", "value": true}
}
```


**Pitfalls:**
- When calling `startInternalClock` from a MIDI callback (`onNoteOn`), always pass `Message.getTimestamp()` instead of 0. The timestamp provides sub-block accuracy -- using 0 introduces up to one buffer's worth of timing jitter (e.g., ~5.8ms at 512 samples / 44.1kHz).
