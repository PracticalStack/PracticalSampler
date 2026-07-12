## fill

**Examples:**

```javascript:initialize-key-state-display
// Title: Initialize a key-state display to "all keys up"
// Context: A visual keyboard panel uses 0 for "key up" and 1 for "key down."
// fill(0) establishes the initial state where no keys are pressed.

const var noteStates = Engine.createMidiList();
noteStates.fill(0);

// Later, in onNoteOn / onNoteOff:
// noteStates.setValue(Message.getNoteNumber(), 1);  // key down
// noteStates.setValue(Message.getNoteNumber(), 0);  // key up
```
```json:testMetadata:initialize-key-state-display
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "noteStates.getValue(0)", "value": 0},
    {"type": "REPL", "expression": "noteStates.getValue(127)", "value": 0},
    {"type": "REPL", "expression": "noteStates.getNumSetValues()", "value": 128}
  ]
}
```

```javascript:initialize-per-note-timer
// Title: Initialize a per-note timer to zero
// Context: A round-robin system needs to track when each note was last
// triggered. fill(0) ensures that the first note-on for any key will
// always exceed the reset threshold (since Engine.getUptime() > 0).

const var lastTriggerTime = Engine.createMidiList();
lastTriggerTime.fill(0);

const var RESET_THRESHOLD = 3; // seconds

// In onNoteOn:
// local elapsed = Engine.getUptime() - lastTriggerTime.getValue(noteNumber);
// if (elapsed > RESET_THRESHOLD)
//     resetRoundRobin();
// lastTriggerTime.setValue(noteNumber, Engine.getUptime());
```
```json:testMetadata:initialize-per-note-timer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastTriggerTime.getValue(60)", "value": 0},
    {"type": "REPL", "expression": "lastTriggerTime.getNumSetValues()", "value": 128}
  ]
}
```

Note the distinction: `fill(0)` sets all slots to zero (useful for counters and flags), while `clear()` sets all slots to `-1` (the "unset" sentinel). Choose based on whether 0 or -1 is the meaningful default for your use case.
