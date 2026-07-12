## store

**Examples:**

```javascript:release-sample-triggering
// Title: Release sample triggering with held note tracking
// Context: A sampler that plays release samples when notes are released.
// store() copies the note-on event into a MessageHolder for later replay
// as a release trigger. An UnorderedStack tracks all held notes.

const var holder = Engine.createMessageHolder();
const var stack = Engine.createUnorderedStack();
stack.setIsEventStack(true, stack.EventId);

const var uptimes = [];
uptimes.reserve(128);

for (i = 0; i < 128; i++)
    uptimes[i] = 0.0;

// Register a callback to clear state when all notes are released
inline function onAllOff()
{
    stack.clear();

    for (i = 0; i < 128; i++)
        uptimes[i] = 0.0;
}

Message.setAllNotesOffCallback(onAllOff);

inline function handleNoteOn()
{
    // Store the current event for later release triggering
    Message.store(holder);
    stack.insert(holder);
    uptimes[Message.getNoteNumber()] = Engine.getUptime();

    // Suppress the note-on from this sampler (it plays release samples only)
    Message.ignoreEvent(true);
}

inline function handleNoteOff()
{
    Message.store(holder);
    stack.removeIfEqual(holder);

    // Calculate note duration to choose short vs long release sample
    local duration = Engine.getUptime() - uptimes[holder.getNoteNumber()];
    local useShort = duration < 0.8;

    Sampler.setActiveGroup(useShort ? 1 : 2);
    holder.setGain(-6);

    // Replay the stored event as a release trigger
    Synth.addMessageFromHolder(holder);
}
```
```json:testMetadata:release-sample-triggering
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

The `store()` -> `MessageHolder` -> `Synth.addMessageFromHolder()` pipeline is the standard way to capture a MIDI event for deferred replay. The holder preserves the full event including velocity, channel, transpose, and gain modifications.
