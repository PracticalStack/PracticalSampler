FixObjectStack (object)
Obtain via: factory.createStack(numElements)

Variable-occupancy preallocated stack of typed objects with insert, remove, and
duplicate detection. Tracks an internal position pointer separating used elements
from unused capacity. All operations are allocation-free after creation.

Constants:
  Capacity:
    length = (constructor arg)    Total allocated capacity passed to createStack

Complexity tiers:
  1. Basic tracking: insert, remove, size, clear. Add events on note-on, remove
     on note-off, check size for capacity.
  2. Animated visualization: + copy, removeElement, for...in iteration. Extract
     property columns into Buffers, expire events during iteration.
  3. Full lifecycle management: + sort, fill, toBase64/fromBase64. Manual capacity
     management, serialization, ordering guarantees after swap-and-pop removal.

Practical defaults:
  - Use capacity 128 when tracking per-note events -- matches MIDI note range
    with headroom for overlapping events.
  - Set a custom compare function on the unique ID field (e.g., event ID) rather
    than comparing all properties. Allows updating other properties without
    breaking duplicate detection.
  - Create a single reusable temp object via factory.create() and mutate its
    properties before each insert(). Avoids creating new objects per event.

Common mistakes:
  - Creating a new FixObject for every insert -- defeats the allocation-free
    design. Reuse a single temp object: mutate properties, then insert().
  - for (i = 0; i < stack.size(); i++) { stack.removeElement(i); } -- swap-and-pop
    moves the last element into the removed slot. Use removeElement(i--) to avoid
    skipping the swapped-in element.
  - Relying on element order after removeElement() -- swap-and-pop does not
    preserve insertion order. Call sort() after modifications if order matters.
  - copy() reads all allocated slots including unused ones -- use a manual loop
    from 0 to size() to copy only the used portion.
  - toBase64()/fromBase64() do not save or restore the position pointer -- save
    size() separately and re-insert or track the used count externally.

Example:
  // Create a factory and stack for tracking active notes
  const var f = Engine.createFixObjectFactory({
      "note": 0,
      "velocity": 0.0,
      "active": false
  });

  f.setCompareFunction("note");
  const var s = f.createStack(16);

Methods (15):
  clear            clearQuick       contains
  copy             fill             fromBase64
  indexOf          insert           isEmpty
  remove           removeElement    set
  size             sort             toBase64
