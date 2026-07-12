## insert

**Examples:**

```javascript:reusable-temp-object-pattern
// Title: Reusable temp object pattern for event tracking
// Context: Real-time note tracking uses a single temp object to avoid
// per-event allocation. Mutate properties, then insert() copies the data.

const var f = Engine.createFixObjectFactory({
    "eventId": -1,
    "start": -1.0,
    "end": -1.0,
    "key": 0,
    "velocity": 0,
    "active": false
});

inline function compareById(a, b)
{
    local id1 = a.eventId;
    local id2 = b.eventId;
    if (id2 < id1) return -1;
    if (id2 > id1) return 1;
    return 0;
}

f.setCompareFunction(compareById);
const var stack = f.createStack(128);
const var temp = f.create();

// Reuse temp for each note-on -- insert() copies data into the stack
inline function onNoteOn(eventId, noteNumber, vel)
{
    // Manual capacity management: evict oldest entry when near full
    if (stack.size() >= 127)
    {
        local oldestIdx = 0;
        local oldestStart = 0.0;

        for (i = 0; i < stack.size(); i++)
        {
            if (stack[i].start > oldestStart)
            {
                oldestIdx = i;
                oldestStart = stack[i].start;
            }
        }

        stack.removeElement(oldestIdx);
    }

    temp.eventId = eventId;
    temp.key = noteNumber;
    temp.velocity = vel;
    temp.start = 0.0;
    temp.end = -1.0;
    temp.active = true;
    stack.insert(temp);
}

// Demonstrate the pattern
onNoteOn(100, 60, 80);
onNoteOn(101, 64, 100);
onNoteOn(102, 67, 90);

Console.print(stack.size()); // 3
```
```json:testMetadata:reusable-temp-object-pattern
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "stack.size()", "value": 3},
    {"type": "REPL", "expression": "stack[0].key", "value": 60},
    {"type": "REPL", "expression": "stack[1].velocity", "value": 100},
    {"type": "REPL", "expression": "stack[2].active", "value": true}
  ]
}
```

**Pitfalls:**
- When the stack is near capacity, insert() silently overwrites the last element rather than failing. In production, check `size()` before inserting and manually evict entries to stay within safe bounds.
