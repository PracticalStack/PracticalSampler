# Array -- Project Context

## Project Context

### Real-World Use Cases
- **MIDI event processing**: A step sequencer or arpeggiator uses arrays of MessageHolder objects, then calls `filter()` and `find()` to extract note-on events within time ranges or locate specific note-off events by event ID. The functional methods turn raw MIDI lists into per-step data for UI display.
- **Audio-thread-safe collections**: A unison voice engine or release trigger script pre-allocates arrays with `reserve(128)` in onInit, then uses `push`/`pop`/`removeElement` in MIDI callbacks without triggering reallocation warnings. Nested arrays (e.g., per-note event ID lists) each get their own `reserve` call.
- **Automation data assembly**: A plugin with many automatable parameters builds its full automation list by creating sub-arrays per channel strip, then calling `concat()` repeatedly to merge them into one master array. The in-place behavior of `concat` makes this a natural accumulation pattern.
- **File and sample name management**: A sound browser or preset selector collects directory names, deduplicates with `pushIfNotAlreadyThere`, and displays them sorted with `sortNatural()` for correct ordering of names containing numbers.

### Complexity Tiers
1. **Basic collection** (most common): `push`, `pop`, `contains`, `indexOf`, `clear`, `join`, `length` - simple data storage and lookup. Every plugin uses arrays at this level.
2. **Data processing**: `sort`, `sortNatural`, `filter`, `find`, `map`, `clone`, `concat` - transforming and querying structured data. Used in preset management, sound browsers, and MIDI processing.
3. **Audio-thread patterns**: `reserve` + `push`/`pop`/`removeElement` with pre-allocated capacity. Required for any array modified inside `onNoteOn`, `onNoteOff`, or `onController` callbacks.

### Practical Defaults
- Use `reserve(128)` for arrays modified on the audio thread - 128 is the standard capacity matching MIDI note count.
- Use `sortNatural()` for file names and sample names. Use `sort()` only for purely numeric arrays.
- Use `for (x in array)` instead of `forEach()` on the audio thread - the for-in loop does not allocate scope objects.
- Use `clone()` when creating multiple objects from a template - assignment only copies the reference.
- Use `pushIfNotAlreadyThere()` when collecting items from parsed data where duplicates are expected.

### Integration Patterns
- `Array.filter()` with `MessageHolder.getTimestamp()` / `MessageHolder.isNoteOn()` - extracting MIDI events within a time range from a MIDI player's event list.
- `Array.reserve()` in onInit then `Array.push()` in onNoteOn - pre-allocating for audio-thread-safe accumulation of event IDs or note tracking data.
- `Array.concat()` in a loop to assemble `UserPresetHandler` automation data from per-channel sub-arrays.
- `Array.sortNatural()` after file collection for display in a `ScriptedViewport` or tag list UI.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Modifying arrays in MIDI callbacks without `reserve()` | Call `reserve(n)` in onInit before any audio-thread mutations | Each `push` that exceeds capacity triggers a reallocation warning and potential audio glitch. |
| Using `forEach()` on the audio thread | Use `for (x in array)` loop | `forEach` allocates scope objects internally. The for-in loop is allocation-free. |
| `removeElement(i)` in a forward loop without adjusting the index | `removeElement(i--)` or iterate backward | Removing shifts elements left, so the next element moves to the current index and gets skipped. |
| Using `sort()` on an array of file names | Use `sortNatural()` | Default `sort()` only compares numerically. String elements all compare as equal and remain unsorted. |
