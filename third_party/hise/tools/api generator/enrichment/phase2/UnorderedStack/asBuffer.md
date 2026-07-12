## asBuffer

**Examples:**

```javascript:shader-uniform-full-array
// Title: Feeding shader uniform data with the full backing array
// Context: OpenGL shaders expect fixed-size arrays. asBuffer(true)
// returns all 128 slots, providing a stable buffer size regardless
// of how many elements are in the stack.

const var keyState = Engine.createUnorderedStack();
const var shader = Content.createShader("keyboard");

// Note events update the stack
inline function handleNote(noteNumber, velocity)
{
    if (velocity != 0)
        keyState.insert(noteNumber);
    else
        keyState.remove(noteNumber);

    // Pass the full 128-slot array to the shader
    shader.setUniformData("keyState", keyState.asBuffer(true));
}
```
```json:testMetadata:shader-uniform-full-array
{
  "testable": false,
  "skipReason": "Content.createShader() requires a registered shader file"
}
```

```javascript:iterate-active-elements
// Title: Iterating active elements for chord analysis
// Context: asBuffer(false) returns only occupied elements, ideal
// for iterating active notes without processing 128 empty slots.

const var pressedNotes = Engine.createUnorderedStack();

inline function analyzeChord()
{
    if (pressedNotes.size() < 3)
        return;

    // Iterate only the occupied elements
    local activeBuffer = pressedNotes.asBuffer(false);
    local pitchClasses = [];

    for (note in activeBuffer)
    {
        local pc = parseInt(note) % 12;
        pitchClasses.push(pc);
    }

    Console.print("Pitch classes: " + trace(pitchClasses));
}

// --- test-only ---
pressedNotes.insert(60.0);
pressedNotes.insert(64.0);
pressedNotes.insert(67.0);
analyzeChord();
// --- end test-only ---
```
```json:testMetadata:iterate-active-elements
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "pressedNotes.size()", "value": 3},
    {"type": "REPL", "expression": "pressedNotes.asBuffer(false).length", "value": 3}
  ]
}
```

**Pitfalls:**
- The buffer returned by `asBuffer(false)` is a live view - its size changes after every `insert()` or `remove()`. Do not cache the length before a mutation and iterate past the new bounds.
- Writing to `asBuffer(false)` modifies the stack's backing array directly. This can be used intentionally (e.g., decaying float values in a timer callback) but is a subtle side effect if unintended.
