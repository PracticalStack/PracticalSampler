## loadFromXmlFile

**Examples:**

```javascript:load-rsa-key-xml
// Title: Load an RSA key pair from an XML file
// Context: Reading RSA keys for product activation workflows

const var projectRoot = FileSystem.getFolder(FileSystem.AudioFiles).getParentDirectory();
const var keyFile = projectRoot.getChildFile("RSA.xml");

if (keyFile.isFile())
{
    var keys = keyFile.loadFromXmlFile();

    // Always validate that the expected properties exist
    if (isDefined(keys) && isDefined(keys.PublicKey))
        Console.print("Public key loaded");
    else
        Console.print("RSA file is corrupt or missing keys");
}
```
```json:testMetadata:load-rsa-key-xml
{
  "testable": false,
  "skipReason": "Requires an RSA.xml file in the project root directory"
}
```

```javascript:scan-preset-metadata
// Title: Parse preset XML files to extract metadata
// Context: Scanning preset files to build a searchable database

const var presetDir = FileSystem.getFolder(FileSystem.UserPresets);
var presetFiles = FileSystem.findFiles(presetDir, "*.preset", true);

for (f in presetFiles)
{
    var presetData = f.loadFromXmlFile();

    if (isDefined(presetData) && isDefined(presetData.CustomJSON))
    {
        // Extract metadata from the preset's custom data section
        Console.print("Preset: " + f.toString(1));
    }
}
```
```json:testMetadata:scan-preset-metadata
{
  "testable": false,
  "skipReason": "Requires preset files (.preset) in the UserPresets directory"
}
```

**Pitfalls:**
- Returns `undefined` silently when the file does not exist, is empty, or contains invalid XML. Unlike `loadAsObject` (which reports a parse error), this method fails silently. Always guard with `isDefined()`.
