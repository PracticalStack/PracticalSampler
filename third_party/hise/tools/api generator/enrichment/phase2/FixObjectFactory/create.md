## create

**Examples:**

```javascript:reusable-temp-for-stack-insertion
// Title: Reusable temp object for stack insertion
// Context: Avoid per-event allocation by creating one template object and reusing it

const var factory = Engine.createFixObjectFactory({
    "eventId": -1,
    "noteNumber": 0,
    "velocity": 0,
    "startTime": 0.0,
    "active": false
});

const var stack = factory.createStack(128);
const var temp = factory.create();

// Set up identity-based comparison for lookups
factory.setCompareFunction("eventId");

// In a note-on handler: populate temp and insert
inline function handleNoteOn(id, number, vel)
{
    temp.eventId = id;
    temp.noteNumber = number;
    temp.velocity = vel;
    temp.startTime = 0.0;
    temp.active = true;

    stack.insert(temp);
}

// In a note-off handler: find and update by eventId
inline function handleNoteOff(id)
{
    for (obj in stack)
    {
        if (obj.eventId == id)
        {
            obj.active = false;
            break;
        }
    }
}

handleNoteOn(1, 60, 100);
handleNoteOn(2, 64, 80);
Console.print(stack.size()); // 2

handleNoteOff(1);

// Iterate to verify state
for (obj in stack)
    Console.print("Note " + obj.noteNumber + " active: " + obj.active);
// Note 60 active: 0
// Note 64 active: 1
```
```json:testMetadata:reusable-temp-for-stack-insertion
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["2", "Note 60 active: 0", "Note 64 active: 1"]}
  ]
}
```

**Pitfalls:**
- The temp object pattern requires overwriting ALL properties before each `insert()`. If you forget to reset a property, the previous event's value leaks into the new entry. Initialize every field explicitly, not just the ones that changed.
