## writeAsXmlFile

**Examples:**

```javascript:persist-settings-xml
// Title: Persist plugin settings as XML alongside presets
// Context: Settings that survive preset changes (e.g., graphics quality, UI preferences)

const var settingsFile = FileSystem.getFolder(FileSystem.AppData).getChildFile("Settings.xml");

// Load existing settings or start fresh
var settings = settingsFile.loadFromXmlFile();

if (!isDefined(settings))
    settings = {};

// Update a setting
settings["GRAPHICS_QUALITY"] = 2;
settings["OPEN_GL_ENABLED"] = true;

// Write back with a root tag name matching the filename
settingsFile.writeAsXmlFile(settings, "Settings");
```
```json:testMetadata:persist-settings-xml
{
  "testable": false,
  "skipReason": "Writes to AppData filesystem"
}
```

**Cross References:**
- `File.loadFromXmlFile`
