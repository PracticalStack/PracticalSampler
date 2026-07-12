## getCustomProperties

**Examples:**

```javascript:custom-props-peak-analysis
// Title: Peak analysis workflow with sortable metadata
// Context: Measure peak loudness of each sample, attach it as custom
// metadata, then sort by peak to assign velocity zones. The metadata
// travels with the Sample object through sorting.

const var sampler = Synth.getSampler("Sampler1");
const var selection = sampler.createSelection(".*");

// Phase 1: Analyze peaks and attach to each sample
var minDb = 0.0;
var maxDb = -100.0;

for (s in selection)
{
    local buf = s.loadIntoBufferArray()[0];
    local peak = Engine.getDecibelsForGainFactor(buf.getMagnitude());

    // Attach peak as transient metadata - not saved to sample map
    s.getCustomProperties().Peak = peak;
    minDb = Math.min(minDb, peak);
    maxDb = Math.max(maxDb, peak);
}

// Store global range on each sample for normalization
for (s in selection)
{
    s.getCustomProperties().MinPeak = minDb;
    s.getCustomProperties().MaxPeak = maxDb;
}

// Phase 2: Sort by peak (custom properties survive the sort)
inline function comparePeaks(s1, s2)
{
    local p1 = s1.getCustomProperties().Peak;
    local p2 = s2.getCustomProperties().Peak;

    if (p1 > p2) return 1;
    if (p2 > p1) return -1;
    return 0;
}

Engine.sortWithFunction(selection, comparePeaks);

// Phase 3: Assign velocity zones based on sorted order
for (i = 0; i < selection.length; i++)
{
    local loVel = parseInt(i * (128 / selection.length));
    local hiVel = parseInt((i + 1) * (128 / selection.length)) - 1;

    selection[i].set(Sample.LoVel, loVel);
    selection[i].set(Sample.HiVel, hiVel);
}

Sampler.refreshInterface();
```

```json:testMetadata:custom-props-peak-analysis
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map"
}
```
