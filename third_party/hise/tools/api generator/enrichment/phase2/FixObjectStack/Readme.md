# FixObjectStack -- Project Context

## Project Context

### Real-World Use Cases
- **Real-time event lifecycle tracker**: A plugin that needs to visualize active MIDI notes (or any transient events) uses FixObjectStack to track note-on/note-off state with per-event metadata (start time, end time, sustain phase, velocity). A timer callback animates the data each frame, removes expired events, and bulk-copies property columns into Buffers for rendering (e.g., as shader uniforms or paint routine data). The stack's insert/remove semantics with duplicate detection by event ID make it a natural fit for tracking a variable number of concurrent events without heap allocation.

### Complexity Tiers
1. **Basic tracking** (most common): `insert`, `remove`, `size`, `clear` - add events on note-on, remove on note-off, check size for capacity. Uses a reusable temp object to avoid per-event allocation.
2. **Animated visualization**: Adds `copy()` to extract property columns into Buffers, `removeElement()` with index adjustment for expiring events during iteration, and `for...in` iteration to mutate live event data each frame.
3. **Full lifecycle management**: Adds manual capacity management (evicting the oldest event when near capacity), sustain pedal state tracking per event, and sentinel values in unused slots for downstream consumers that read the full capacity.

### Practical Defaults
- Use capacity 128 when tracking per-note events - it matches the MIDI note range and provides headroom for overlapping events.
- Set a custom compare function on the unique ID field (e.g., event ID) rather than comparing all properties. This allows updating other properties without breaking duplicate detection.
- Create a single reusable temp object via `factory.create()` and mutate its properties before each `insert()`. This avoids creating new objects per event.

### Integration Patterns
- `FixObjectStack.copy()` -> `Buffer` -> `ScriptShader.setUniformData()` - Extract per-element property columns into Buffers and feed them as shader uniform arrays for GPU-accelerated visualization.
- `ScriptPanel.setTimerCallback()` -> iterate stack with `for...in` -> `removeElement()` expired entries -> `copy()` to Buffers -> `repaint()` - A timer-driven animation loop that mutates event data, cleans up expired entries, and feeds the rendering pipeline each frame.
- `FixObjectFactory.setCompareFunction()` -> `FixObjectStack.insert()` - The factory's compare function determines duplicate semantics for all stack operations. Set it before creating the stack or immediately after.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new FixObject for every insert | Reuse a single temp object: mutate properties, then `insert()` | `insert()` copies the data into the stack's preallocated slot. Creating new objects per event defeats the allocation-free design. |
| `for (i = 0; i < stack.size(); i++) { stack.removeElement(i); }` | `stack.removeElement(i--);` after removal | `removeElement()` uses swap-and-pop: the last element moves into the removed slot. Without decrementing `i`, the swapped-in element is skipped. |
| Relying on element order after `removeElement()` | Call `sort()` after modifications if order matters | Swap-and-pop removal does not preserve insertion order. If downstream code depends on ordering (e.g., oldest-first), sort explicitly. |
