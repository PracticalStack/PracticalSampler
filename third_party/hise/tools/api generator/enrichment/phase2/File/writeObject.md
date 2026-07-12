## writeObject

**Examples:**

```javascript:incremental-settings-update
// Title: Write a settings key without overwriting the entire file
// Context: Incremental settings updates that preserve existing keys

const var settingsFile = FileSystem.getFolder(FileSystem.AppData).getChildFile("settings.json");

inline function writeSetting(key, value)
{
    local data = {};

    // Load existing settings first to preserve other keys
    if (settingsFile.isFile())
        data = settingsFile.loadAsObject();

    data[key] = value;
    settingsFile.writeObject(data);
}

writeSetting("theme", 2);
writeSetting("zoom", 1.5);
// File now contains: {"theme": 2, "zoom": 1.5}
```
```json:testMetadata:incremental-settings-update
{
  "testable": false,
  "skipReason": "Writes to AppData filesystem"
}
```

```javascript:save-config-browse-dialog
// Title: Save and load a structured configuration file
// Context: Persisting a configuration object that maps to UI state

const var configDir = FileSystem.getFolder(FileSystem.AppData);

// Build the data to save
var routingConfig = {
    "Channels": [1, 2, 3, 4],
    "SendFX": [0, 0, 1, 1]
};

// Save to a user-chosen file via browse dialog
FileSystem.browse(FileSystem.AppData, true, "*.json", function(f)
{
    f.writeObject(routingConfig);
});
```
```json:testMetadata:save-config-browse-dialog
{
  "testable": false,
  "skipReason": "Opens a native file browse dialog requiring user interaction"
}
```
