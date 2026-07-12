## createDirectory

**Examples:**

```javascript:create-preset-collection
// Title: Create a user content directory with metadata
// Context: Building a custom preset collection folder

const var presetRoot = FileSystem.getFolder(FileSystem.UserPresets);

inline function createCollection(name, author)
{
    local dir = presetRoot.createDirectory(name);

    local metadata = {
        "heading": name,
        "creator": author,
        "description": "Custom collection"
    };

    dir.getChildFile("info.json").writeObject(metadata);

    return dir;
}

var newCollection = createCollection("My Presets", "User");
Console.print(newCollection.toString(0));
```
```json:testMetadata:create-preset-collection
{
  "testable": false,
  "skipReason": "Creates directories and files in UserPresets filesystem"
}
```

```javascript:ensure-data-directories
// Title: Ensure a data directory exists before first use
// Context: Initializing app-specific storage on first launch

const var appData = FileSystem.getFolder(FileSystem.AppData);

// createDirectory is safe to call if the directory already exists
const var settingsDir = appData.createDirectory("Settings");
const var cacheDir = appData.createDirectory("Cache");
```
```json:testMetadata:ensure-data-directories
{
  "testable": false,
  "skipReason": "Creates directories in AppData filesystem"
}
```
