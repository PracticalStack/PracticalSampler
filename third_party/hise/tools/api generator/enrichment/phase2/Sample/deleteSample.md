## deleteSample

**Examples:**

```javascript:delete-and-remap
// Title: Delete a sample and remap a replacement to its position
// Context: In a sample curation tool, remove an unwanted sample from
// the map and reassign another sample to fill its key position.

const var sampler = Synth.getSampler("Sampler1");

// Get the sample to delete (e.g., from GUI selection)
const var toDelete = Sampler.createListFromGUISelection()[0];

if (isDefined(toDelete))
{
    // Remember the key position before deletion
    local noteNumber = toDelete.get(Sample.Root);

    // Find a replacement sample (e.g., the last sample in the map)
    const var allSamples = sampler.createSelection(".*");
    var replacement = -1;

    for (s in allSamples)
    {
        // Skip the sample we're about to delete
        if (s.refersToSameSample(toDelete))
            continue;

        replacement = s;
    }

    // Delete the unwanted sample
    toDelete.deleteSample();
    // toDelete is now invalid - do not use it again

    // Remap the replacement to the deleted sample's position
    if (replacement != -1)
    {
        replacement.set(Sample.Root, noteNumber);
        replacement.set(Sample.LoKey, noteNumber);
        replacement.set(Sample.HiKey, noteNumber);
    }

    Sampler.refreshInterface();
}
```

```json:testMetadata:delete-and-remap
{
  "testable": false,
  "skipReason": "Requires a loaded sampler with samples in the sample map; destructive operation"
}
```

**Pitfalls:**
- The deletion is deferred (voices are killed first via `killAllVoicesAndCall`). The Sample reference becomes invalid after the call returns, but the actual removal happens asynchronously. Discard the reference immediately and do not use it in subsequent iterations of a loop.
