## setAllNotesOffCallback

**Examples:**

```javascript:reset-held-notes-on-all-off
// Title: Resetting held-note tracking on all-notes-off
// Context: A release sample player tracks held notes using an
// UnorderedStack. When the host sends an all-notes-off message,
// the callback clears the tracking data to prevent stuck notes
// or orphaned release triggers.

const var stack = Engine.createUnorderedStack();
stack.setIsEventStack(true, stack.EventId);

reg pedalDown = false;

const var uptimes = [];
uptimes.reserve(128);

for (i = 0; i < 128; i++)
    uptimes[i] = 0.0;

// The callback runs on the audio thread -- must be an inline function
inline function onAllOff()
{
    stack.clear();
    pedalDown = false;

    for (i = 0; i < 128; i++)
        uptimes[i] = 0.0;
}

Message.setAllNotesOffCallback(onAllOff);
```
```json:testMetadata:reset-held-notes-on-all-off
{
  "testable": false,
  "skipReason": "Requires AllNotesOff MIDI event which cannot be triggered programmatically from script"
}
```

AllNotesOff events do not trigger `onNoteOff` or `onController` -- this callback is the only way to receive them. Any script that tracks note state (held notes, sustain pedal, active voice counts) should register this callback to reset cleanly.
