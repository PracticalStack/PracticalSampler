<!-- Diagram triage:
  - No class-level or method-level diagrams in the JSON. Nothing to render.
-->

# UnorderedStack

UnorderedStack is a lock-free, fixed-capacity container that holds up to 128 elements with set semantics - insertions reject duplicates and removal is O(1). Create one with `Engine.createUnorderedStack()`.

The stack operates in one of two modes:

| Mode | Element type | Default | Use case |
|------|-------------|---------|----------|
| Float | Numbers | Yes | Tracking active note numbers, feeding shader data |
| Event | MessageHolder objects | No | Preserving full MIDI event metadata through the note lifecycle |

Float mode covers most use cases: tracking which notes are held, checking membership in paint routines, and passing note state to shaders via a buffer view. Switch to event mode only when you need the original note-on event's full metadata (velocity, timestamp, event ID) at note-off time.

In event mode, a compare function controls how `contains()`, `remove()`, and `removeIfEqual()` match events:

| Constant | Value | Match criteria |
|----------|-------|----------------|
| `UnorderedStack.BitwiseEqual` | 0 | All event fields must be identical |
| `UnorderedStack.EventId` | 1 | Event ID only (pairs note-on/off) |
| `UnorderedStack.NoteNumberAndVelocity` | 2 | Same note number and velocity |
| `UnorderedStack.NoteNumberAndChannel` | 3 | Same note number and channel |

You can also pass a custom inline function instead of a constant. It receives two MessageHolder arguments (the stack element and the search target) and returns true for a match.

```js
const var us = Engine.createUnorderedStack();

// Event mode with EventId matching
us.setIsEventStack(true, us.EventId);
```

> [!Tip:Bracket reads float array, no write support] Bracket access (`stack[index]`) reads from the float array even when the stack is in event mode. Writing via bracket is not supported. Element order is not preserved - removal fills gaps by swapping in the last element.

## Common Mistakes

- **Call setIsEventStack before insert**
  **Wrong:** `us.insert(messageHolder)` without calling `setIsEventStack` first
  **Right:** Call `us.setIsEventStack(true, us.EventId)` before inserting events
  *Float mode is the default. Inserting a MessageHolder into a float-mode stack silently returns false with no error.*

- **Reuse MessageHolder as temp object**
  **Wrong:** Creating a new MessageHolder in every note-on callback
  **Right:** Create one `const var holder = Engine.createMessageHolder()` in onInit and reuse it
  *MessageHolder creation allocates on the heap. Reuse a single holder and overwrite it with `Message.store(holder)` each time.*

- **Use asBuffer(true) for writeable copy**
  **Wrong:** Using `asBuffer(false)` for shader uniform data
  **Right:** Use `asBuffer(true)` for shaders
  *Shaders expect fixed-size arrays. `asBuffer(false)` changes size dynamically as elements are added or removed, causing shader indexing errors.*

- **Drain from index 0 in while loop**
  **Wrong:** Iterating with an ascending index-based for loop while removing elements
  **Right:** Drain from index 0 in a `while (!stack.isEmpty())` loop using `storeEvent(0, holder)` + `removeElement(0)`
  *Removal swaps in the last element, so forward iteration skips the element that moves into the vacated slot.*
