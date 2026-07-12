## deleteFileOrDirectory

**Examples:**

```javascript:toggle-default-file
// Title: Toggle a default state file on/off
// Context: Using file existence as a boolean flag for "default active" state

const var defaultFile = FileSystem.getFolder(FileSystem.AppData).getChildFile("defaults.json");

inline function toggleDefault(currentState, stateObject)
{
    if (currentState)
    {
        // Remove the default file to "deactivate"
        defaultFile.deleteFileOrDirectory();
    }
    else
    {
        // Save current state as the new default
        defaultFile.writeObject(stateObject);
    }
}

// Create a default, then toggle it off
toggleDefault(false, {"volume": 0.8});
toggleDefault(true, {});
```
```json:testMetadata:toggle-default-file
{
  "testable": false,
  "skipReason": "Modifies AppData filesystem (writes and deletes defaults.json)"
}
```

```javascript:invalidate-rebuild-cache
// Title: Clear a cached database before rebuilding
// Context: Invalidating a cached JSON file when source data changes

const var cacheFile = FileSystem.getFolder(FileSystem.AppData).getChildFile("cache.json");

inline function invalidateCache()
{
    cacheFile.deleteFileOrDirectory();
}

inline function getOrBuildCache()
{
    if (cacheFile.isFile())
        return cacheFile.loadAsObject();

    // Rebuild cache from source data...
    var freshData = {"rebuilt": true};
    cacheFile.writeObject(freshData);
    return freshData;
}

// First call builds cache, second loads it, invalidate forces rebuild
var data1 = getOrBuildCache();
var data2 = getOrBuildCache();
invalidateCache();
var data3 = getOrBuildCache();
```
```json:testMetadata:invalidate-rebuild-cache
{
  "testable": false,
  "skipReason": "Modifies AppData filesystem (writes and deletes cache.json)"
}
```
