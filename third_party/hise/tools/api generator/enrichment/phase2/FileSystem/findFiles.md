## findFiles

**Examples:**

```javascript:scan-sample-library-structure
// Title: Scan a sample library's directory structure
// Context: A plugin with user-selectable audio content scans a hierarchical
// folder structure (Category -> Type -> files) and builds a metadata array.
// Results are cached to avoid re-scanning on every launch.

const var AUDIO_WILDCARD = "*.wav,*.aif";

inline function scanContentLibrary()
{
    local sampleRoot = FileSystem.getFolder(FileSystem.Samples);

    if (!isDefined(sampleRoot))
        return [];

    local contentFolder = sampleRoot.getChildFile("Content");

    if (!contentFolder.isDirectory())
        return [];

    // First level: category folders (e.g., "Kicks", "Snares", "HiHats")
    local categories = FileSystem.findFiles(contentFolder, "*", false);
    local database = [];

    for (cat in categories)
    {
        if (!cat.isDirectory())
            continue;

        local categoryName = cat.toString(1);

        // Second level: type folders (e.g., "Acoustic", "Electronic")
        local typeFolders = FileSystem.findFiles(cat, "*", false);

        for (typeFolder in typeFolders)
        {
            if (!typeFolder.isDirectory())
                continue;

            local typeName = typeFolder.toString(1);

            // Third level: actual audio files
            local audioFiles = FileSystem.findFiles(typeFolder, AUDIO_WILDCARD, true);

            for (sample in audioFiles)
            {
                database.push({
                    "Name": sample.toString(1),
                    "Category": categoryName,
                    "Type": typeName,
                    "Path": sample.toString(0)
                });
            }
        }
    }

    return database;
}

// Cache the scan results to avoid rescanning on every launch
inline function getCacheFile()
{
    return FileSystem.getFolder(FileSystem.AppData).getChildFile("contentCache.json");
}

var contentData = getCacheFile().loadAsObject();

if (!isDefined(contentData) || contentData.length == 0)
{
    contentData = scanContentLibrary();
    getCacheFile().writeObject(contentData);
}
```
```json:testMetadata:scan-sample-library-structure
{
  "testable": false,
  "skipReason": "Depends on project-specific Samples directory structure with Content/Category/Type subfolders. Results vary by machine and project."
}
```

```javascript:build-categorized-ir-list
// Title: Build a categorized IR file list for a reverb selector
// Context: A reverb plugin organizes impulse responses in subfolders by
// category. findFiles enumerates each level to build a structured list
// for a dropdown or table view.

inline function buildIRList()
{
    local audioFolder = FileSystem.getFolder(FileSystem.Samples);

    if (!isDefined(audioFolder))
        return {};

    local irRoot = audioFolder.getChildFile("IR Files");

    if (!irRoot.isDirectory())
        return {};

    local categories = FileSystem.findFiles(irRoot, "*", false);
    local result = {};

    for (c in categories)
    {
        if (!c.isDirectory())
            continue;

        local catName = c.toString(1);
        local irFiles = FileSystem.findFiles(c, "*.wav", false);
        local entries = [];

        for (i = 0; i < irFiles.length; i++)
        {
            local thisFile = irFiles[i];
            // Estimate duration from file size (16-bit stereo at 48kHz)
            local durationMs = (thisFile.getSize() / 6) / 48;

            entries.push({
                "Name": thisFile.toString(1),
                "Path": thisFile.toString(0),
                "Duration": durationMs
            });
        }

        result[catName] = entries;
    }

    return result;
}
```
```json:testMetadata:build-categorized-ir-list
{
  "testable": false,
  "skipReason": "Depends on project-specific Samples directory with 'IR Files' subdirectory structure. Results vary by machine and project."
}
```

**Pitfalls:**
- `findFiles()` returns both files and directories that match the wildcard. When scanning for audio files, the results may include subdirectories whose names happen to match the pattern. Filter with `File.isDirectory()` if you only want files, or be specific with the extension wildcard.
- On large content libraries (thousands of files across deep directory trees), `findFiles()` with `recursive = true` can take noticeable time. Cache the results to a JSON file in AppData and only rescan when the user explicitly requests it.
