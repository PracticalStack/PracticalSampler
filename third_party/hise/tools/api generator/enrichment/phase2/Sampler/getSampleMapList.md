## getSampleMapList

**Examples:**

```javascript:category-browser
// Title: Building a category browser from sample map folder structure
// Context: Sample maps stored in subfolders (e.g., "Pads/Warm", "Leads/Bright")
// can be parsed into a two-column category + sound browser.

const var sampler = Synth.getSampler("MainSampler");

inline function buildSoundLibrary()
{
    local mapList = Sampler.getSampleMapList();
    local data = {};

    // Extract categories from folder paths
    for (mapId in mapList)
    {
        local parts = mapId.split("/");
        
        // Skip maps without a category folder
        if (parts.length < 2)
            continue;

        local category = parts[0];
        local soundName = parts[1];

        if (!isDefined(data[category]))
            data[category] = [];

        data[category].push(soundName);
    }

    return data;
}

const var soundLibrary = buildSoundLibrary();

// Load a sound by category and name
inline function loadSound(category, soundName)
{
    local mapId = category + "/" + soundName;
    sampler.loadSampleMap(mapId);
}
```

```json:testMetadata:category-browser
{
  "testable": false,
  "skipReason": "Requires sample maps in project pool"
}
```

The returned array contains pool reference strings sorted alphabetically. Maps in subfolders use `"Folder/MapName"` format. Maps at the root level have no separator.

**Cross References:**
- `Sampler.loadSampleMap`
- `Sampler.getCurrentSampleMapId`
