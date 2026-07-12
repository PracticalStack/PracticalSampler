## setActiveGroup

**Examples:**

```javascript:duration-based-group
// Title: Duration-based release group switching
// Context: A release sampler selects between short and long release samples
// based on how long the note was held. Short notes (< 0.8s) trigger group 1,
// longer notes trigger group 2.

Sampler.enableRoundRobin(false);
Sampler.setActiveGroup(2); // Default to long release

const var uptimes = Engine.createMidiList();
const var holder = Engine.createMessageHolder();

// In onNoteOn: record the note start time
inline function handleNoteOn()
{
    uptimes.setValue(Message.getNoteNumber(), Engine.getUptime());
    Message.store(holder);
    Message.ignoreEvent(true);
}

// In onNoteOff: switch group based on hold duration, then replay
inline function handleNoteOff()
{
    Message.store(holder);

    local duration = Engine.getUptime() - uptimes.getValue(holder.getNoteNumber());
    local isShort = duration < 0.8;

    // Select the appropriate release group before triggering
    Sampler.setActiveGroup(isShort ? 1 : 2);

    holder.setGain(-6);
    Synth.addMessageFromHolder(holder);
}
```

```json:testMetadata:duration-based-group
{
  "testable": false,
  "skipReason": "Requires sampler with RR groups, note-on/off callbacks"
}
```

The group index is one-based. `setActiveGroup(1)` selects the first group. This method sets the group globally - for per-event group selection, use `setActiveGroupForEventId()` from within `onNoteOn`.
