## setGlobalPlaybackRatio

**Examples:**

```javascript:global-speed-via-broadcaster
// Title: Global speed control via broadcaster
// Context: Use a broadcaster to update the global playback ratio across
// all MidiPlayer instances when a speed control changes

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");

const var speedBroadcaster = Engine.createBroadcaster({
    "id": "globalSpeed",
    "args": ["ratio"]
});

speedBroadcaster.addListener(mp, "update playback speed", function(ratio)
{
    this.setGlobalPlaybackRatio(ratio);
});

speedBroadcaster.sendSyncMessage([0.5]);
```
```json:testMetadata:global-speed-via-broadcaster
{
  "testable": false,
  "skipReason": "Global playback ratio is not readable via script API; no getter to verify the set value"
}
```

**Pitfalls:**
- This is a global setting on the MainController, not per-player. Calling it on any MidiPlayer instance affects ALL MidiPlayer instances. The effective speed is `perPlayerSpeed * globalRatio`.
