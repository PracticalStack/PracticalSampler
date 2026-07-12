## asSampler

**Examples:**

```javascript:articulation-sample-maps
// Title: Loading sample maps on articulation child synths
// Context: A multi-articulation instrument stores child synths by name
// and uses asSampler() to access Sampler-specific methods like
// loadSampleMap() and enableRoundRobin().

const var articulationIds = ["sustain", "staccato", "transitions"];
const var childSynths = {};

for (id in articulationIds)
    childSynths[id] = Synth.getChildSynth(id);

inline function loadPatch(patchName)
{
    for (id in articulationIds)
    {
        // asSampler() returns undefined if the child is not a ModulatorSampler
        local sampler = childSynths[id].asSampler();

        if (isDefined(sampler))
            sampler.loadSampleMap(patchName + "_" + id);
    }
}

// Configure round-robin on a specific articulation
childSynths["staccato"].asSampler().enableRoundRobin(false);
```
```json:testMetadata:articulation-sample-maps
{
  "testable": false,
  "skipReason": "Requires named ModulatorSampler child synths and sample maps in the project"
}
```

**Pitfalls:**
- Always guard `asSampler()` with `isDefined()` when the child synth type is not guaranteed. In architectures where some children are SynthGroups and others are ModulatorSamplers, the cast returns undefined silently for non-sampler types.
