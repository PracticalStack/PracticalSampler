## getFolder

**Examples:**

```javascript:persist-settings-to-appdata
// Title: Persist settings to AppData as JSON
// Context: Plugins commonly store user preferences (theme, zoom, MIDI mappings)
// in a JSON file within the AppData directory. This pattern shows the typical
// read-modify-write cycle.

inline function writeSettingToFile(key, value)
{
    local f = FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json");
    local data = {};

    // Load existing settings if the file exists
    if (isDefined(f) && f.isFile())
        data = f.loadAsObject();

    data[key] = value;
    f.writeObject(data);
}

inline function readSettingFromFile(key, defaultValue)
{
    local f = FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json");
    local data = {};

    if (isDefined(f) && f.isFile())
        data = f.loadAsObject();

    local v = data[key];

    if (isDefined(v))
        return v;

    return defaultValue;
}

// Usage
writeSettingToFile("zoomLevel", 1.5);
var zoom = readSettingFromFile("zoomLevel", 1.0);
Console.print("Zoom: " + zoom); // 1.5
```
```json:testMetadata:persist-settings-to-appdata
{
  "testable": false,
  "skipReason": "Writes a settings.json file to AppData as a side effect. While technically runnable, it modifies persistent state on disk which is inappropriate for automated testing."
}
```

```javascript:favorites-system-user-presets
// Title: Build a favorites system using UserPresets folder
// Context: A favorites file stored alongside presets tracks which
// presets the user has marked. The file lives in the UserPresets
// directory so it travels with the preset library.

namespace Favorites
{
    inline function getFile()
    {
        return FileSystem.getFolder(FileSystem.UserPresets).getChildFile("favorites.json");
    }

    inline function load()
    {
        local list = getFile().loadAsObject();

        if (isDefined(list))
            return list;

        return [];
    }

    inline function toggle(relativePath)
    {
        local list = load();

        if (list.indexOf(relativePath) == -1)
            list.push(relativePath);
        else
            list.remove(relativePath);

        getFile().writeObject(list);
        return list;
    }
}
```
```json:testMetadata:favorites-system-user-presets
{
  "testable": false,
  "skipReason": "Writes a favorites.json file to the UserPresets folder as a side effect. Modifies persistent state on disk which is inappropriate for automated testing."
}
```

**Pitfalls:**
- `getFolder(FileSystem.Samples)` returns `undefined` when FullInstrumentExpansion is active but no expansion is currently loaded. Any code that chains `.getChildFile()` on the result will fail. Guard with `isDefined()` before use in plugins that support expansion-based sample folders.
