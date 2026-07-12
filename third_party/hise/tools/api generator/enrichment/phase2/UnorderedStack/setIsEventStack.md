## setIsEventStack

**Examples:**

```javascript:event-tracking-release
// Title: Event-mode stack for tracking held notes with release triggering
// Context: A release sample player tracks note-on events so that when
// a note-off arrives, it can recover the original event's metadata
// (velocity, timestamp) to trigger an appropriate release sample.

const var noteStack = Engine.createUnorderedStack();
const var holder = Engine.createMessageHolder();

// Configure as event stack with EventId matching (pairs note-on/off)
noteStack.setIsEventStack(true, noteStack.EventId);

// In onNoteOn: store the current event and push it onto the stack
inline function handleNoteOn()
{
    Message.store(holder);
    noteStack.insert(holder);
}

// In onNoteOff: pop the matching note-on event from the stack
inline function handleNoteOff()
{
    Message.store(holder);

    // removeIfEqual finds the matching event by ID and writes
    // the original note-on data back into the holder
    noteStack.removeIfEqual(holder);

    // holder now contains the original note-on event - use its
    // velocity, timestamp, etc. for release sample selection
    Synth.addMessageFromHolder(holder);
}
```
```json:testMetadata:event-tracking-release
{
  "testable": false,
  "skipReason": "Callback functions use Message.store() and Synth.addMessageFromHolder() which require MIDI processing context"
}
```

**Pitfalls:**
- When switching from float mode to event mode, the float stack's data is not cleared. Always call `clear()` if switching modes after data has been inserted (though mode should generally be set once during initialization).
