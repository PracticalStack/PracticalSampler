## split

**Examples:**

```javascript:parse-sample-map-id
// Title: Parse a hierarchical sample map ID into folder and name
// Context: Sample map IDs use "/" as a path separator. Splitting
// extracts the category and the individual sample name.

var sampleMapId = "Drums/Kicks/HardKick";
var parts = sampleMapId.split("/");

var category = parts[0];  // "Drums"
var subFolder = parts[1]; // "Kicks"
var name = parts[2];      // "HardKick"
Console.print(category + " > " + name); // Drums > HardKick
```
```json:testMetadata:parse-sample-map-id
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Drums > HardKick"]}
  ]
}
```

```javascript:version-string-migration
// Title: Parse a version string for migration logic
// Context: User presets store a version string. On load, split it
// into components to decide whether migration is needed.

inline function needsMigration(version)
{
    local v = version.split(".");
    
    // Check if major version is 1 and minor is less than 2
    if (parseInt(v[0]) == 1 && parseInt(v[1]) < 2)
        return true;
    
    return false;
}

Console.print(needsMigration("1.1.3")); // 1 (true)
Console.print(needsMigration("1.2.0")); // 0 (false)
```
```json:testMetadata:version-string-migration
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["1", "0"]}
  ]
}
```

```javascript:parse-text-file-lines
// Title: Parse a text file into lines for data import
// Context: Loading a legacy data file as a string, then splitting
// on newlines to process each line individually.

const var f = FileSystem.getFolder(FileSystem.Documents).getChildFile("data.txt");
var lines = f.loadAsString().split("\n");

for (line in lines)
{
    local values = line.split(",");
    // Process each comma-separated value...
}
```
```json:testMetadata:parse-text-file-lines
{
  "testable": false,
  "skipReason": "Requires an external data.txt file in the Documents folder"
}
```

**Pitfalls:**
- Only the first character of the separator is used. `"a::b::c".split("::")` splits on `":"`, producing `["a", "", "b", "", "c"]` instead of the expected `["a", "b", "c"]`. Use a single-character delimiter.
