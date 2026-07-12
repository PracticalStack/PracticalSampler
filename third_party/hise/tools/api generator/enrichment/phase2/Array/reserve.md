## reserve

**Examples:**

```javascript:pre-allocate-audio-thread
// Title: Pre-allocate arrays for audio-thread safety
// Context: Any array that will be modified in onNoteOn/onNoteOff
// must be pre-allocated in onInit. This prevents reallocation
// warnings and potential audio glitches.

// Simple flat array for tracking uptimes per note
const var uptimes = [];
uptimes.reserve(128);

for(i = 0; i < 128; i++)
    uptimes[i] = 0.0;

// Nested arrays: each sub-array needs its own reserve call
const var eventIds = [];
eventIds.reserve(128);

for(i = 0; i < 128; i++)
{
    eventIds[i] = [];
    eventIds[i].reserve(16);
}

// Now push/pop in MIDI callbacks stays within allocated capacity
function onNoteOn()
{
    eventIds[Message.getNoteNumber()].push(Message.getEventId());
    uptimes[Message.getNoteNumber()] = Engine.getUptime();
}

function onNoteOff()
{
    local note = Message.getNoteNumber();
    local ids = eventIds[note];

    // pop from pre-allocated array -- no reallocation
    if(!ids.isEmpty())
        ids.pop();
}
```
```json:testMetadata:pre-allocate-audio-thread
{
  "testable": false,
  "skipReason": "MIDI callbacks (onNoteOn/onNoteOff) require MIDI input to trigger"
}
```

**Pitfalls:**
- `reserve` only sets capacity, not length. The array remains empty after `reserve` -- you still need to populate it with `push` or index assignment.
- For nested arrays, you must `reserve` each inner array individually. `reserve(128)` on the outer array only allocates slots for 128 references, not capacity within each sub-array.
