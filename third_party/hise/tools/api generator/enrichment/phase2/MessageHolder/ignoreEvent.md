## ignoreEvent

**Examples:**

```javascript:release-sample-store-ignore-reinject
// Title: Release sample triggering - store, ignore, re-inject on NoteOff
// Context: A sampler that intercepts NoteOn events, stores them for later,
// and re-injects modified copies when the key is released. The hold duration
// determines which sample group to trigger.

const var holder = Engine.createMessageHolder();
const var uptimes = [];
uptimes.reserve(128);

for (i = 0; i < 128; i++)
    uptimes[i] = 0.0;

// Called from onNoteOn callback
inline function handleNoteOn()
{
    Message.store(holder);
    uptimes[Message.getNoteNumber()] = Engine.getUptime();

    // Suppress the original NoteOn - we'll trigger our own later
    Message.ignoreEvent(true);
}

// Called from onNoteOff callback
inline function handleNoteOff()
{
    // Store the NoteOff into the same holder (captures note number for matching)
    Message.store(holder);

    local duration = Engine.getUptime() - uptimes[holder.getNoteNumber()];
    local useShortRelease = duration < 0.8;

    // Select sample group based on how long the note was held
    Sampler.setActiveGroup(useShortRelease ? 1 : 2);
    holder.setGain(-6);

    Synth.addMessageFromHolder(holder);
}
```
```json:testMetadata:release-sample-store-ignore-reinject
{
  "testable": false,
  "skipReason": "Requires MIDI callbacks (onNoteOn/onNoteOff) with Message.store(), Message.ignoreEvent(), and Synth.addMessageFromHolder()"
}
```

**Pitfalls:**
- The `ignoreEvent()` method on MessageHolder modifies only the stored copy's flag, not the live event in the audio buffer. To suppress the original event during a MIDI callback, call `Message.ignoreEvent(true)` (on the `Message` object), not `holder.ignoreEvent(true)`. The MessageHolder version is only useful when preparing events that will later be re-injected and should arrive pre-flagged as ignored.
