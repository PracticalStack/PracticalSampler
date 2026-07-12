## setKeyColour

**Examples:**

```javascript:colour-coded-keyboard-zones
// Title: Colour-coding keyboard zones for a multi-layer sampler
// Context: Sampler instruments with multiple layers or split key ranges
// use setKeyColour to give visual feedback about which notes are mapped.
// Always iterate all 128 keys to avoid stale colours from previous states.

const var sampler1 = Synth.getSampler("Sampler1");
const var sampler2 = Synth.getSampler("Sampler2");

inline function refreshKeyColours()
{
    for (i = 0; i < 127; i++)
    {
        local c = 0x00000000;

        // Layer 1 mapped notes get a blue tint
        if (sampler1.isNoteNumberMapped(i))
            c = c | 0x220077FF;

        // Layer 2 mapped notes get an orange tint
        if (sampler2.isNoteNumberMapped(i))
            c = c | 0x22FF8800;

        // Unmapped keys get a dimmed-out colour
        if (c == 0)
            c = 0x99444444;

        Engine.setKeyColour(i, c);
    }
}

// Refresh after samples finish loading
const var bgPanel = Content.getComponent("Background");

bgPanel.setLoadingCallback(function(isPreloading)
{
    if (!isPreloading)
        refreshKeyColours();
});
```
```json:testMetadata:colour-coded-keyboard-zones
{
  "testable": false,
  "skipReason": "Requires Sampler modules (Sampler1, Sampler2) and a Background panel in the module tree."
}
```

**Pitfalls:**
- Colours use bitwise OR (`|`) to blend layers. If two samplers overlap on the same note, both tints are combined into a mixed colour. This is intentional -- it gives visual feedback about overlapping ranges.
