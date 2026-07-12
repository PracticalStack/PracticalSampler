## setGain

**Examples:**

```javascript:per-event-gain-before-reinjection
// Title: Applying per-event gain adjustment before re-injection
// Context: When re-injecting stored events (e.g., release samples triggered
// on NoteOff), setGain() attenuates the event independently of the original
// velocity. This separates volume control from velocity-based sample selection.

const var holder = Engine.createMessageHolder();

inline function handleNoteOff()
{
    Message.store(holder);

    // Reduce the release sample volume by 6 dB relative to the original
    holder.setGain(-6);

    Synth.addMessageFromHolder(holder);
}
```
```json:testMetadata:per-event-gain-before-reinjection
{
  "testable": false,
  "skipReason": "Requires onNoteOff callback with Message.store() and Synth.addMessageFromHolder() - cannot be triggered from onInit"
}
```

`setGain()` is applied as a per-voice gain factor during rendering, separate from velocity. This means a NoteOn with velocity 100 and gain -6 dB triggers the velocity=100 sample layer but plays it 6 dB quieter. Use this when you want to control volume without changing which sample layer gets selected.
