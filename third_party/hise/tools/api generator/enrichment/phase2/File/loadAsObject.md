## loadAsObject

**Examples:**

```javascript:first-run-fallback
// Title: Load JSON with first-run fallback
// Context: Persistent settings that may not exist on first plugin launch

const var settingsFile = FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json");

inline function readSetting(key, defaultValue)
{
    local f = settingsFile;
    local data = {};

    // File may not exist on first run
    if (isDefined(f) && f.isFile())
        data = f.loadAsObject();

    local v = data[key];

    if (isDefined(v))
        return v;
    else
        return defaultValue;
}

Console.print(readSetting("theme", 0)); // 0 (default on first run)
```
```json:testMetadata:first-run-fallback
{
  "testable": false,
  "skipReason": "Depends on AppData filesystem state; result varies between first run and subsequent runs"
}
```

```javascript:load-favorites-list
// Title: Load a favorites list as JSON array
// Context: Managing a favorites list stored alongside presets

inline function loadFavorites()
{
    local f = FileSystem.getFolder(FileSystem.UserPresets).getChildFile("favorites.json");
    local list = f.loadAsObject();

    // Return empty array if file doesn't exist yet
    if (isDefined(list))
        return list;

    return [];
}

var favorites = loadFavorites();
Console.print("Favorites: " + favorites.length);
```
```json:testMetadata:load-favorites-list
{
  "testable": false,
  "skipReason": "Depends on UserPresets filesystem state; loadAsObject reports script error if file exists but contains invalid JSON"
}
```

**Pitfalls:**
- On first plugin launch the settings file will not exist. `loadAsObject` reports a script error if the file content is invalid JSON, but returns `undefined` for non-existent files. Always guard with `isDefined()` or check `isFile()` first.
