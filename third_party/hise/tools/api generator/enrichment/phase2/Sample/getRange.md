## getRange

**Examples:**

```javascript:getrange-loop-bounds
// Title: Reading dynamic bounds for loop point UI controls
// Context: Loop property ranges depend on other property values. Query
// getRange() to map a normalized UI slider (0-1) to the valid range.

const var sampler = Synth.getSampler("Sampler1");
const var sound = sampler.createSelection(".*")[0];

// Total sample length for a progress-style display
const var totalSamples = sound.getRange(Sample.SampleEnd)[1];

// Current valid crossfade range depends on LoopStart and SampleStart
local xfadeRange = sound.getRange(Sample.LoopXFade);
Console.print("XFade range: " + xfadeRange[0] + " - " + xfadeRange[1]);

// Map a 0-1 slider value to the valid crossfade range
inline function applyNormalizedXFade(normalizedValue)
{
    local maxXFade = sound.getRange(Sample.LoopXFade)[1];
    // Re-query each time - the range changes when LoopStart moves
    local newValue = parseInt(maxXFade * normalizedValue);
    sound.set(Sample.LoopXFade, newValue);
}
```

```json:testMetadata:getrange-loop-bounds
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```

**Pitfalls:**
- Ranges are dynamic and interdependent. Caching `getRange()` results at init time and reusing them after property changes leads to stale bounds. Re-query after any `set()` call that affects related properties (e.g., re-query `LoopXFade` range after changing `LoopStart`).
