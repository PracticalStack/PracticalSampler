# UnorderedStack -- Project Context

## Project Context

### Real-World Use Cases
- **Active note tracker (float mode)**: A plugin that needs to track which notes are currently held uses a float-mode UnorderedStack as a lightweight set of note numbers. Notes are inserted on note-on and removed on note-off, with `contains()` used in paint routines to highlight active keys and `asBuffer()` to iterate held notes for analysis (chord detection, shader visualization). This is the most common use case - nearly all observed usage follows this pattern.
- **MIDI event lifecycle manager (event mode)**: A sampler that implements custom release triggering uses an event-mode stack to track held note events. When a note-off arrives, `removeIfEqual()` pops the matching event (preserving the original note-on's metadata like velocity and timestamp), which is then used to trigger a release sample with parameters derived from the held duration and original velocity.
- **GPU visualization data source (float mode)**: A plugin with OpenGL shader-based UI uses `asBuffer(true)` to pass the full 128-slot backing array directly to a shader as uniform data, providing a fixed-size buffer the shader can index by note number without needing to know which slots are occupied.

### Complexity Tiers
1. **Float note tracker** (most common): `insert`, `remove`, `contains`, `clear`, `size`. Simple set operations on note numbers - sufficient for keyboard highlighting, chord detection, and basic note state tracking.
2. **Buffer-backed visualization**: Adds `asBuffer(false)` for iterating active elements and `asBuffer(true)` for fixed-size shader data. Enables real-time visual feedback driven by note state.
3. **Event-mode MIDI lifecycle**: Uses `setIsEventStack`, `insert` with MessageHolder, `removeIfEqual` for pop-matching, `storeEvent`/`removeElement` for sequential drain. Required when the original note-on event metadata (velocity, timestamp, event ID) must be preserved through the note lifecycle.

### Practical Defaults
- Use float mode (the default) for note number tracking. Event mode is only needed when you must recover the original note-on event's full metadata at note-off time.
- Use `EventId` as the compare function for event-mode stacks that track note-on/off pairs. This is the natural match since HISE assigns matching event IDs to paired note events.
- Create a single MessageHolder with `Engine.createMessageHolder()` and reuse it across callbacks. There is no need to create a new holder per event - `Message.store(holder)` overwrites the holder's content each time.
- Use `asBuffer(true)` (all 128 slots) when feeding shader uniforms - shaders expect fixed-size arrays. Use `asBuffer(false)` when iterating only occupied elements in script logic.

### Integration Patterns
- `UnorderedStack.insert()` / `UnorderedStack.remove()` in `onNoteOn` / `onNoteOff` -> `UnorderedStack.contains()` in `ScriptPanel.setPaintRoutine()` -- drives keyboard highlighting by checking note membership during repaint.
- `Message.store(holder)` -> `UnorderedStack.insert(holder)` in `onNoteOn` -> `UnorderedStack.removeIfEqual(holder)` in `onNoteOff` -> `Synth.addMessageFromHolder(holder)` -- full event lifecycle where the popped event drives release sample triggering.
- `UnorderedStack.asBuffer(true)` -> `ScriptShader.setUniformData()` -- passes the 128-slot backing array as shader uniform data for GPU-accelerated note visualization.
- `UnorderedStack.asBuffer(false)` iteration -> analysis logic (chord detection, note selection) -- iterates only occupied slots for algorithmic processing.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new MessageHolder in every note-on callback | Create one `const var holder = Engine.createMessageHolder()` in onInit and reuse it | MessageHolder creation allocates on the heap. Reuse a single holder and overwrite it with `Message.store(holder)` each time. |
| Using `asBuffer(false)` for shader uniform data | Use `asBuffer(true)` for shaders | Shaders expect fixed-size arrays. `asBuffer(false)` changes size dynamically as elements are added/removed, causing shader indexing errors. |
| Iterating with index-based for loop over `asBuffer(false)` while removing elements | Use a `while(!stack.isEmpty())` drain loop with `storeEvent(0, holder)` + `removeElement(0)` | Removing elements during forward iteration skips entries because removal swaps in the last element. Drain from index 0 in a while loop instead. |
