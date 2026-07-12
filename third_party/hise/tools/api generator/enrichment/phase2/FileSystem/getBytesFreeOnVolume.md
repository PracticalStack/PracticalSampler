## getBytesFreeOnVolume

**Examples:**

```javascript:validate-disk-space-before-install
// Title: Validate disk space before a sample installation
// Context: Before extracting or downloading large sample libraries, check
// that the target volume has enough free space. Combine with
// descriptionOfSizeInBytes() for user-friendly messages.

const var REQUIRED_BYTES = 2147483648; // 2 GB minimum

inline function checkDiskSpace(targetFolder)
{
    local freeBytes = FileSystem.getBytesFreeOnVolume(targetFolder);
    local freeText = FileSystem.descriptionOfSizeInBytes(freeBytes);
    local requiredText = FileSystem.descriptionOfSizeInBytes(REQUIRED_BYTES);

    if (freeBytes < REQUIRED_BYTES)
    {
        Engine.showMessageBox(
            "Insufficient Disk Space",
            "The target volume has " + freeText + " free, but " + requiredText + " is required.",
            3
        );
        return false;
    }

    return true;
}

// Usage in an installer flow
inline function onInstallClicked(component, value)
{
    if (!value) return;

    local targetPath = Content.getComponent("TargetInput").getValue();
    local targetFolder = FileSystem.fromAbsolutePath(targetPath);

    if (isDefined(targetFolder) && checkDiskSpace(targetFolder))
        startInstallation(targetFolder);
};
```
```json:testMetadata:validate-disk-space-before-install
{
  "testable": false,
  "skipReason": "References undefined startInstallation() function and Content.getComponent('TargetInput') which does not exist. Demonstrates an integration flow that cannot run standalone."
}
```
