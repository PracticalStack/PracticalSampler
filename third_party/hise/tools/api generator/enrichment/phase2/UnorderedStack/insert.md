## insert

**Examples:**

```javascript:float-note-tracking
// Title: Float-mode note tracking with random selection
// Context: A granular effect tracks active notes and randomly selects
// from them to spawn new grains. insert() provides set semantics
// (no duplicates), and bracket access picks a random element.

const var activeNotes = Engine.createUnorderedStack();

// onNoteOn / onNoteOff
inline function handleNote(noteNumber, velocity)
{
    if (velocity != 0)
        activeNotes.insert(noteNumber);
    else
        activeNotes.remove(noteNumber);
}

// Timer callback: pick a random held note for grain spawning
inline function spawnGrain()
{
    local count = activeNotes.size();

    if (count > 0)
    {
        // Bracket access reads from the float array by index
        local randomNote = activeNotes[Math.randInt(0, count)];
        Console.print("Spawning grain at note: " + randomNote);
    }
}

// --- test-only ---
handleNote(60, 100);
handleNote(64, 100);
handleNote(67, 100);
handleNote(64, 0);
// --- end test-only ---
```
```json:testMetadata:float-note-tracking
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "activeNotes.size()", "value": 2},
    {"type": "REPL", "expression": "activeNotes.contains(60.0)", "value": 1},
    {"type": "REPL", "expression": "activeNotes.contains(64.0)", "value": 0},
    {"type": "REPL", "expression": "activeNotes.contains(67.0)", "value": 1}
  ]
}
```

```javascript:event-mode-insert
// Title: Event-mode insertion with MessageHolder
// Context: Inserting events requires a MessageHolder. The current
// MIDI event is captured with Message.store(), then the holder
// is passed to insert(). Duplicate detection uses the configured
// compare function.

const var eventStack = Engine.createUnorderedStack();
const var holder = Engine.createMessageHolder();
eventStack.setIsEventStack(true, eventStack.EventId);

// onNoteOn
inline function handleNoteOn()
{
    Message.store(holder);
    eventStack.insert(holder); // returns false if event ID already present
}

// --- test-only ---
holder.setNoteNumber(60);
holder.setVelocity(100);
eventStack.insert(holder);
// --- end test-only ---
```
```json:testMetadata:event-mode-insert
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "eventStack.size()", "value": 1}
}
```
