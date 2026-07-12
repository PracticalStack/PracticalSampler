## writeString

**Examples:**

```javascript:write-sample-link-file
// Title: Write a HISE sample folder link file
// Context: After extracting samples to a user-chosen location, create a link file
//          so HISE redirects its Samples folder to that location

inline function writeLinkFile(targetFolder)
{
    local appFolder;

    if (Engine.isHISE())
        appFolder = FileSystem.getFolder(FileSystem.AudioFiles).getParentDirectory().getChildFile("Samples");
    else
        appFolder = FileSystem.getFolder(FileSystem.AppData);

    // Platform-specific link file name
    local fileName = Engine.getOS() == "WIN" ? "LinkWindows" : "LinkOSX";
    local linkFile = appFolder.getChildFile(fileName);

    // The link file contains just the absolute path as plain text
    linkFile.writeString(targetFolder.toString(0));
}

// Redirect Samples folder to an external drive
var externalDrive = FileSystem.getFolder(FileSystem.Desktop).getChildFile("SampleLibrary");
writeLinkFile(externalDrive);
```
```json:testMetadata:write-sample-link-file
{
  "testable": false,
  "skipReason": "Writes a link file to AppData/project directory which would redirect HISE folder resolution"
}
```
