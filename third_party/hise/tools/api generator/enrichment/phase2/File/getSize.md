## getSize

**Examples:**

```javascript:validate-file-sizes
// Title: Validate downloaded file sizes before installation
// Context: Checking that ZIP files are complete before extraction

const var expectedFiles = [
    {"name": "Samples_01.zip", "size": 1291959296},
    {"name": "Samples_02.zip", "size": 1831375285}
];

inline function validateFiles(sourceFolder)
{
    for (entry in expectedFiles)
    {
        local f = sourceFolder.getChildFile(entry.name);

        if (!f.isFile())
            return "Missing file: " + entry.name;

        if (f.getSize() != entry.size)
            return entry.name + " is corrupt (expected " + entry.size + " bytes, got " + f.getSize() + ")";
    }

    return ""; // All files valid
}

// Check files in the Downloads folder
var result = validateFiles(FileSystem.getFolder(FileSystem.Downloads));
Console.print(result.length == 0 ? "All files valid" : result);
```
```json:testMetadata:validate-file-sizes
{
  "testable": false,
  "skipReason": "Requires specific ZIP files on disk with known sizes"
}
```
