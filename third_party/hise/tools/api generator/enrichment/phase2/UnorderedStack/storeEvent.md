## storeEvent

**Examples:**

```javascript:drain-on-pedal-off
// Title: Drain loop for releasing all held notes on pedal-off
// Context: When a sustain pedal is released, all events in the stack
// must be processed and removed. storeEvent reads without removing,
// then removeElement clears the slot. Draining from index 0 avoids
// the index-shifting problems of forward iteration with removal.

const var noteStack = Engine.createUnorderedStack();
const var holder = Engine.createMessageHolder();
const var processedNotes = Engine.createMidiList();
noteStack.setIsEventStack(true, noteStack.EventId);

inline function drainOnPedalOff()
{
    processedNotes.clear();

    while (!noteStack.isEmpty())
    {
        // Read the event at index 0 into the holder
        noteStack.storeEvent(0, holder);

        // Remove index 0 (last element swaps in, stack shrinks by 1)
        noteStack.removeElement(0);

        // Deduplicate: skip if this note number was already processed
        // (handles edge case of multiple events for the same pitch)
        if (processedNotes.getValue(holder.getNoteNumber()) == -1)
        {
            processedNotes.setValue(holder.getNoteNumber(), 1);
            Synth.addMessageFromHolder(holder);
        }
    }
}
```
```json:testMetadata:drain-on-pedal-off
{
  "testable": false,
  "skipReason": "Synth.addMessageFromHolder() requires an active MIDI processing context"
}
```

**Pitfalls:**
- Do not use a for loop with ascending index when draining: `removeElement` fills the gap by swapping in the last element, so the element that moves to the removed index would be skipped. Always drain from index 0 in a while loop.

**Cross References:**
- `UnorderedStack.removeElement`
- `UnorderedStack.isEmpty`
