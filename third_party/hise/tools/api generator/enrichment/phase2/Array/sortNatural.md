## sortNatural

**Examples:**

```javascript:sort-file-names-natural
// Title: Sort file names with embedded numbers
// Context: When displaying sample or preset names collected from
// the file system, sortNatural() ensures correct ordering where
// "Kit 2" comes before "Kit 10".

const var folders = FileSystem.getFolder(FileSystem.AudioFiles);
const var files = FileSystem.findFiles(folders, "*.wav", false);

const var names = [];

for(f in files)
{
    // Collect directory names, avoiding duplicates
    names.pushIfNotAlreadyThere(f.toString(1));
}

names.sortNatural();

// Result: ["Kit 1", "Kit 2", "Kit 10", "Kit 20"]
// sort() would give: ["Kit 1", "Kit 10", "Kit 2", "Kit 20"] -- wrong
Console.print(names.join(", "));
```
```json:testMetadata:sort-file-names-natural
{
  "testable": false,
  "skipReason": "Requires audio files on disk in the project's AudioFiles folder"
}
```
