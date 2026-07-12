## browseForDirectory

**Examples:**

```javascript:relocate-sample-folder
// Title: Let the user relocate the sample content folder
// Context: When a plugin cannot find its sample content (e.g., after a fresh
// install or drive change), it opens a directory browser so the user can
// point to the correct location. The selected folder is validated before
// being accepted.

inline function showSampleFolderDialog()
{
    FileSystem.browseForDirectory(FileSystem.Samples, function(newFolder)
    {
        // Validate that the chosen folder contains expected content
        if (newFolder.getChildFile("Content").isDirectory())
        {
            Settings.setSampleFolder(newFolder);

            // Inform the user that a restart is needed
            Engine.showMessageBox(
                "Relocation OK",
                "New folder: " + newFolder.toString(1) + "\nPlease restart to apply.",
                0
            );
        }
        else
        {
            Engine.showMessageBox(
                "Invalid Folder",
                "The selected folder does not contain the expected content.",
                3
            );
        }
    });
}
```
```json:testMetadata:relocate-sample-folder
{
  "testable": false,
  "skipReason": "Requires native OS directory dialog interaction that cannot be automated."
}
```
