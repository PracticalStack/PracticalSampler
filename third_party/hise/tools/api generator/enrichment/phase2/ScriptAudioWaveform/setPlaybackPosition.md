## setPlaybackPosition

**Examples:**

```javascript:reset-cursor-on-source-switch
// Title: Reset playback cursor when switching audio sources
// Context: When a multi-channel instrument switches which processor
// the waveform displays, the playback cursor retains its old position.
// Reset it to the start after rebinding.

const var wf = Content.addAudioWaveform("Waveform1", 0, 0);

inline function onChannelSwitch(component, value)
{
    local playerIds = ["Player1", "Player2", "Player3", "Player4"];
    local idx = parseInt(value);

    // Rebind the waveform to the new channel's audio processor
    wf.set("processorId", playerIds[idx]);

    // Reset cursor -- without this, the ruler stays at the old position
    wf.setPlaybackPosition(0);

    wf.sendRepaintMessage();
}

// Demonstrate the reset call directly (no audio data needed for the API call)
wf.setPlaybackPosition(0.5);
wf.setPlaybackPosition(0);
```

```json:testMetadata:reset-cursor-on-source-switch
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "wf.getRangeEnd()", "value": 0}
}
```

**Pitfalls:**
- After changing the `processorId` property at runtime, always call `setPlaybackPosition(0)` to reset the cursor. The display does not auto-reset when the data source changes.
