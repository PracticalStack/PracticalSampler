# FixObjectFactory -- Project Context

## Project Context

### Real-World Use Cases
- **Real-time visualization data pipeline**: A plugin that drives GLSL shader visualizations uses FixObjectFactory to define a typed schema for note events (position, velocity, decay state), creates a stack from the factory, and copies property columns into Buffer arrays each timer tick for `ScriptShader.setUniformData()`. The fixed memory layout avoids allocations in the timer loop, and `FixObjectStack.copy()` efficiently extracts per-property columns for the GPU.
- **Particle system state management**: A granular effect visualization tracks active grain particles using a FixObjectArray. Each grain has position, randomness seed, and gain properties. The timer callback iterates the array to decay grain amplitudes and recycle dead grains by overwriting their properties in-place, avoiding insert/remove overhead.

### Complexity Tiers
1. **Basic container** (most common): `createArray()` or `createStack()` with a simple prototype and direct property access in loops. No compare function needed.
2. **Searchable container**: Add `setCompareFunction()` with a property name string for optimized `indexOf`/`contains` lookups and sorting.
3. **Temp object + stack workflow**: Use `create()` to make a reusable template object, populate it per-event, and `insert()` into a stack. Pair with a custom compare function for identity-based removal. This avoids per-event allocation.

### Practical Defaults
- Use `createStack()` when elements are added and removed dynamically (note tracking, particle pools). Use `createArray()` when the element count is fixed and you iterate the full set.
- A capacity of 128 is a good default for note-tracking stacks -- it matches the MIDI note range and the maximum voice count in most configurations.
- Prefer the single-property string comparator (`setCompareFunction("id")`) over a custom function when sorting or searching by one field. It runs entirely in C++ with no script callback overhead.

### Integration Patterns
- `FixObjectFactory.createStack()` -> `FixObjectStack.copy()` -> `Buffer` -> `ScriptShader.setUniformData()` -- the primary pipeline for feeding typed object data to GLSL shaders. The stack acts as a structured data source, `copy()` extracts a single property column into a Buffer, and the Buffer is passed as shader uniform data.
- `FixObjectFactory.create()` -> populate -> `FixObjectStack.insert()` -- reusable temp object pattern. A single FixObject is created once at init time, then reused as a template for every stack insertion by overwriting its properties before each `insert()` call.
- `FixObjectFactory.setCompareFunction()` -> `FixObjectStack.insert()` / `removeElement()` -- the compare function determines identity for duplicate checking on insert and lookup for removal.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new `factory.create()` object for every note-on event | Create one temp object at init, rewrite its properties before each `insert()` | `create()` allocates heap memory. In a note-on handler or timer callback, repeated allocation causes memory pressure. Reuse a single temp object as an insertion template. |
| Using `createArray()` for a dynamically-sized collection | Use `createStack()` for insert/remove workflows | FixObjectArray has a fixed element count with no insert/remove. FixObjectStack tracks active element count and supports `insert()`, `removeElement()`, and `size()`. |
