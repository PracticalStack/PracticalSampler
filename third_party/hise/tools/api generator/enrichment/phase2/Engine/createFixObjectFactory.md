## createFixObjectFactory

**Examples:**

```javascript:fix-object-grain-tracking
// Title: Fixed-layout objects with unordered stack for note tracking
// Context: Performance-critical systems (grain engines, resonance
// models) combine fixed-layout objects for typed per-element data
// with an unordered stack for tracking active notes. This avoids
// heap allocation on the audio thread.

const var factory = Engine.createFixObjectFactory({
    "x": 0,
    "y": 0.0,
    "seed": 0.0,
    "gain": 1.0
});

// Pre-allocate a fixed array of grain descriptors
const var grains = factory.createArray(128);

// Track active notes without allocation
const var activeNotes = Engine.createUnorderedStack();

inline function onNoteOn()
{
    activeNotes.insert(Message.getNoteNumber());
}

inline function onNoteOff()
{
    activeNotes.remove(Message.getNoteNumber());
}

// Spawn a new grain using a random active note
inline function spawnGrain(obj)
{
    local numActive = activeNotes.size();

    if (numActive > 0)
    {
        obj.x = activeNotes[Math.randInt(0, numActive)];
        obj.y = Math.pow(Math.random(), 2.0) * 2.0;
        obj.seed = Math.random();
        obj.gain = Math.pow(1.0 - (obj.y * 0.5), 2.0);
    }
}
```
```json:testMetadata:fix-object-grain-tracking
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "grains.length", "value": 128},
    {"type": "REPL", "expression": "grains[0].x", "value": 0},
    {"type": "REPL", "expression": "grains[0].gain", "value": 1.0},
    {"type": "REPL", "expression": "activeNotes.size()", "value": 0}
  ]
}
```
