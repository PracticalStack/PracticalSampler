<!-- Diagram triage:
  (no diagrams in Phase 1 data)
-->

# FixObjectFactory

FixObjectFactory defines a typed data structure from a JSON prototype and creates memory-efficient containers that conform to that layout. It is particularly useful in MIDI processing contexts where you need a custom data model - tracking note states, chord mappings, keyswitch configurations, or any per-event data that must be stored, searched, and cleared efficiently.

Create a factory by passing a prototype object to `Engine.createFixObjectFactory()`:

```js
const var f = Engine.createFixObjectFactory({
    "eventId": 0,
    "velocity": 0.0,
    "active": false
});
```

Each property becomes a typed member with its value as the default. Supported types:

- Integer (from integer literals like `0`)
- Float (from floating-point literals like `0.0`)
- Boolean (from `true` or `false`)
- Fixed-size numeric array (from array literals like `[0, 0, 0]`)

Strings and nested objects are not supported.

The factory produces three kinds of output: individual objects for use as temporary data buffers, fixed-size arrays for collections with a known element count, and stacks for dynamically-sized collections with insert/remove semantics. All containers are preallocated at creation time, making them safe to use in realtime callbacks. Clearing or resetting a container restores every element to the prototype's default values automatically - particularly convenient when defaults are non-trivial (e.g. a pitch factor defaulting to `1.0` rather than zero).

The factory's comparison function controls how containers perform sorting and lookup. Setting a comparator propagates it retroactively to all containers previously created from that factory. The data layout integrates with IDE tooling: the ScriptWatchTable provides autocomplete for member names, and the StackViewer popup (right-click a stack entry in the ScriptWatchTable and choose **View in Popup**) displays a container's full contents in a table layout. Fix objects can also be passed into eventnode networks, where they are treated as a single data item across the signal graph.

> [!Tip:Shared layout, immutable schema, value copies] All containers share the factory's layout and memory allocator. The schema
> is immutable after construction. Containers operate on value copies, not
> references.

## Common Mistakes

- **Only numeric and boolean types allowed**
  **Wrong:** `Engine.createFixObjectFactory({ "name": "hello" })`
  **Right:** `Engine.createFixObjectFactory({ "id": 0, "value": 0.0 })`
  *String and object values are not valid member types. Only integers, floats, booleans, and arrays of these are supported.*

- **Reuse temp object for insertions**
  **Wrong:** Creating a new `factory.create()` object for every note-on event
  **Right:** Create one temporary object at init time, overwrite its properties before each `insert()`
  *`create()` allocates heap memory. In a note-on handler or timer callback, repeated allocation causes memory pressure. Reuse a single object as an insertion template.*

- **Use createStack for dynamic collections**
  **Wrong:** Using `createArray()` for a dynamically-sized collection
  **Right:** Use `createStack()` for insert/remove workflows
  *FixObjectArray has a fixed element count with no insert or remove operations. FixObjectStack tracks active element count and supports `insert()`, `removeElement()`, and `size()`.*

- **Use function for complex comparisons**
  **Wrong:** `f.setCompareFunction("a,b,c,d,e")`
  **Right:** `f.setCompareFunction(function(a, b) { ... })`
  *Multi-property string comparison is limited to 2-4 properties. Use a custom function for more complex comparisons.*
