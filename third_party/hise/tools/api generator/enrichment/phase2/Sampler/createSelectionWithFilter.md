## createSelectionWithFilter

**Examples:**

```javascript:filter-by-root
// Title: Select all samples mapped to a specific root note
// Context: The filter function receives each sample as `this`, allowing property
// queries via bracket syntax. This is more flexible than regex filename matching.

inline function selectByRoot(noteNumber)
{
    return Sampler.createSelectionWithFilter(function()
    {
        // `this` is a Sample object - use bracket syntax with Sampler constants
        return this[Sampler.Root] == noteNumber;
    });
};

// Select all samples with root note C3 (MIDI note 60)
const var c3Samples = selectByRoot(60);
Console.print("Found " + c3Samples.length + " samples at C3");

// Modify velocity range on the selection
for (i = 0; i < c3Samples.length; i++)
{
    c3Samples[i].set(Sampler.LoVel, i * (128 / c3Samples.length));
    c3Samples[i].set(Sampler.HiVel, (i + 1) * (128 / c3Samples.length) - 1);
}

Sampler.refreshInterface();
```

```json:testMetadata:filter-by-root
{
  "testable": false,
  "skipReason": "Requires sampler with loaded multi-velocity sample map"
}
```

The filter function uses `this` to refer to the current Sample object. The bracket syntax (`this[Sampler.Root]`) is equivalent to `this.get(Sampler.Root)`. Return any non-zero value to include the sample in the result.
