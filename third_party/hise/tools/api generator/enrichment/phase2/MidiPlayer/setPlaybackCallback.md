## setPlaybackCallback

**Examples:**

```javascript:async-transport-ui-update
// Title: Async playback callback for UI state updates
// Context: Register an async (UI-thread) callback to update transport button
// state and handle recording transitions. Use the async mode (0) since
// the callback updates UI components.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.create(4, 4, 1);

reg isRecording = false;
reg lastState = -1;

inline function onPlaybackChange(timestamp, playState)
{
    lastState = playState;

    // playState: 0 = stop, 1 = play, 2 = record
    if (playState == 2 && !isRecording)
    {
        // Just entered recording
        isRecording = true;
    }

    if (playState != 2 && isRecording)
    {
        // Recording just stopped - sequence data was flushed automatically
        isRecording = false;
    }
}

// Pass 0 for async (UI thread) - safe for component updates
mp.setPlaybackCallback(onPlaybackChange, 0);

// --- test-only ---
mp.play(0);
// --- end test-only ---
```
```json:testMetadata:async-transport-ui-update
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "lastState", "value": 1}
  ]
}
```

When using multiple MidiPlayer instances with a shared callback, `this` inside the callback refers to the MidiPlayer that triggered it, allowing you to identify which player changed state.
