## removeElement

**Examples:**

```javascript:safe-forward-iteration
// Title: Safe forward iteration with index adjustment
// Context: When removing elements during a forward loop, decrement
// the index after removal to avoid skipping the next element.

// Remove expired entries from a tracking stack
const var stack = [
    {"progress": 0.5},
    {"progress": 1.2},
    {"progress": 0.8},
    {"progress": 1.5},
    {"progress": 0.3}
];

for(i = 0; i < stack.length; i++)
{
    if(stack[i].progress > 1.0)
        stack.removeElement(i--);
}

Console.print(stack.length); // 3
```
```json:testMetadata:safe-forward-iteration
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["3"]},
    {"type": "REPL", "expression": "stack[1].progress", "value": 0.8}
  ]
}
```

```javascript:evict-oldest-entry
// Title: Evict oldest entry when a stack reaches capacity
// Context: A voice visualization keeps a fixed-size stack of active
// notes. When the stack is full, find the oldest entry and remove
// it by index before inserting the new one.

const var stack = [];
for(i = 0; i < 130; i++)
    stack.push({"elapsed": i});

const var MAX_VOICES = 128;

// Find the entry with the largest elapsed time (oldest)
if(stack.length >= MAX_VOICES)
{
    var maxIdx = 0;
    var maxValue = 0.0;
    var idx = 0;

    for(s in stack)
    {
        if(s.elapsed > maxValue)
        {
            maxIdx = idx;
            maxValue = s.elapsed;
        }

        idx++;
    }

    stack.removeElement(maxIdx);
}

Console.print(stack.length); // 129
```
```json:testMetadata:evict-oldest-entry
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["129"]},
    {"type": "REPL", "expression": "stack[128].elapsed", "value": 128}
  ]
}
```

**Pitfalls:**
- Forgetting `i--` when removing in a forward loop is the most common Array bug in production code. After `removeElement(i)`, element `i+1` shifts to index `i` and gets skipped on the next iteration.
