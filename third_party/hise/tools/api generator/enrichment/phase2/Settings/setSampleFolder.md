## setSampleFolder

**Examples:**

```javascript:sample-folder-relocation
// Title: Sample folder relocation with validation
// Context: When the plugin cannot find its sample content, a dialog
// prompts the user to browse for the correct folder. The selected
// directory is validated before applying.

const var CONTENT_SUBFOLDER = "Samples";

inline function relocateSampleFolder()
{
    FileSystem.browseForDirectory(FileSystem.Samples, function(newFolder)
    {
        if(newFolder.getChildFile(CONTENT_SUBFOLDER).isDirectory())
        {
            Settings.setSampleFolder(newFolder);

            Engine.showMessageBox("Relocation OK",
                "New folder: " + newFolder.toString(1) +
                "\nPlease restart the plugin to apply.", 0);
        }
        else
        {
            Engine.showMessageBox("Invalid Folder",
                "The selected folder does not contain the expected " +
                "content directory.", 0);
        }
    });
}
```

```json:testMetadata:sample-folder-relocation
{
  "testable": false,
  "skipReason": "Requires user interaction with file browser dialog"
}
```
