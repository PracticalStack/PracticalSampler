## getSampleMapList

**Examples:**

```javascript:expansion-sample-map-browser
// Title: Expansion-aware sample map browser
// Context: Build a unified sample map list that includes both embedded
// sample maps and sample maps from each installed expansion.

const var Sampler1 = Synth.getSampler("Sampler1");
const var expHandler = Engine.createExpansionHandler();
const var expansions = expHandler.getExpansionList();

// Add a "Root" entry for embedded sample maps, then each expansion
expansions.insert(0, undefined);

for (e in expansions)
{
    if (isDefined(e))
    {
        var name = e.getProperties().Name;
        var maps = e.getSampleMapList();
        Console.print(name + ": " + maps.length + " sample maps");

        for (m in maps)
            Console.print("  " + m);
    }
    else
    {
        // Embedded sample maps from the project itself
        var rootMaps = Sampler.getSampleMapList();
        Console.print("Root: " + rootMaps.length + " sample maps");
    }
}
```

```json:testMetadata:expansion-sample-map-browser
{
  "testable": false,
  "skipReason": "Requires installed expansion packs with sample maps"
}
```
