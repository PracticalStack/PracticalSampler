## removeIfEqual

**Examples:**

```javascript:pop-matching-release
// Title: Pop-matching for release sample triggering
// Context: When a note-off arrives, removeIfEqual finds the matching
// note-on in the stack and writes the ORIGINAL event back into the holder.
// This preserves the note-on's velocity and timestamp, which may differ
// from the note-off's values.

const var noteStack = Engine.createUnorderedStack();
const var holder = Engine.createMessageHolder();
noteStack.setIsEventStack(true, noteStack.EventId);

reg uptimes = [];
uptimes.reserve(128);

// onNoteOn: push event and record timing
inline function onNoteOn()
{
    Message.store(holder);
    noteStack.insert(holder);
    uptimes[Message.getNoteNumber()] = Engine.getUptime();
}

// onNoteOff: pop matching event, compute held duration
inline function onNoteOff()
{
    Message.store(holder);

    // After this call, holder contains the original note-on event
    noteStack.removeIfEqual(holder);

    // Use the held duration to select short vs long release samples
    local duration = Engine.getUptime() - uptimes[holder.getNoteNumber()];
    local useShortRelease = duration < 0.8;

    // The holder's velocity comes from the original note-on,
    // not the note-off - important for release sample gain scaling
    Synth.addMessageFromHolder(holder);
}
```
```json:testMetadata:pop-matching-release
{
  "testable": false,
  "skipReason": "Callback functions use Message.store() and Synth.addMessageFromHolder() which require MIDI processing context"
}
```

**Pitfalls:**
- The holder is modified in-place even when `removeIfEqual` is called as part of a search-and-process pattern. If you need the search key after the call, save its properties before calling `removeIfEqual`.
