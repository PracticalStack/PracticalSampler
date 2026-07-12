## isNoteNumberMapped

**Examples:**

```javascript:keyboard-colours
// Title: Keyboard colour visualization based on active sample maps
// Context: Highlight playable keys on the virtual keyboard by checking which
// notes have samples mapped. Different colours for each layer, blended for overlap.

const var sampler1 = Synth.getSampler("Layer1");
const var sampler2 = Synth.getSampler("Layer2");

inline function refreshKeyColours()
{
    for (i = 0; i < 127; i++)
    {
        local colour = 0x00000000;

        if (sampler1.isNoteNumberMapped(i))
            colour |= 0x220077FF;  // Semi-transparent blue

        if (sampler2.isNoteNumberMapped(i))
            colour |= 0x22FF8800;  // Semi-transparent orange

        // No samples mapped - dim grey
        if (colour == 0)
            colour = 0x99444444;

        Engine.setKeyColour(i, colour);
    }
}

// Refresh after sample map changes (e.g., in a loading callback)
refreshKeyColours();
```

```json:testMetadata:keyboard-colours
{
  "testable": false,
  "skipReason": "Requires sampler with loaded sample map and keyboard display"
}
```

This method is audio-thread safe - it can be called from any callback. Use bitwise OR (`|=`) to blend semi-transparent colours when multiple layers overlap on the same key.
