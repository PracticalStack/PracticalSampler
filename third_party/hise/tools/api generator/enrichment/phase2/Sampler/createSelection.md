## createSelection

**Examples:**

```javascript:select-and-filter
// Title: Select samples by note name and filter by round-robin group
// Context: Select all samples matching a note name, then filter to a specific
// RR group for targeted property editing.

const var sampler = Synth.getSampler("MainSampler");

inline function selectSamplesForNote(noteName)
{
    // createSelection matches against file names - use the note name as regex
    local allMatches = sampler.createSelection(noteName);

    // Filter to only the first RR group
    local group1 = [];

    for (s in allMatches)
    {
        if (s.get(Sampler.RRGroup) == 1)
            group1.push(s);
    }

    return group1;
}

// Select all "C3" samples in RR group 1
const var c3Samples = selectSamplesForNote("C3");
Console.print("Found " + c3Samples.length + " C3 samples in group 1");
```

```json:testMetadata:select-and-filter
{
  "testable": false,
  "skipReason": "Requires sampler with loaded multi-RR sample map"
}
```

The regex is matched against the full sample file name. Use `".*"` to match all samples. The returned array contains `Sample` objects that can be read/written with `Sample.get()` / `Sample.set()` using the `Sampler.*` property constants.

**Cross References:**
- `Sampler.createSelectionWithFilter`
- `Sampler.createSelectionFromIndexes`
