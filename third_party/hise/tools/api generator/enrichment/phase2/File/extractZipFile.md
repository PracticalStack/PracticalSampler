## extractZipFile

**Examples:**

```javascript:multi-part-zip-installation
// Title: Multi-part sample installation with progress tracking
// Context: Installing a large sample library split across multiple ZIP files

const var sampleFiles = [
    {"FileName": "Samples_01.zip", "FileSize": 1291959296},
    {"FileName": "Samples_02.zip", "FileSize": 1831375285}
];

reg currentPartIndex = -1;
reg installTarget;

inline function onZipProgress(obj)
{
    // Calculate combined progress across all parts
    local partProgress = (1.0 / sampleFiles.length) * obj.Progress;
    local totalProgress = (1.0 / sampleFiles.length) * currentPartIndex;
    local progress = partProgress + totalProgress;

    if (obj.CurrentFile.length > 0)
        Console.print("Installing: " + obj.CurrentFile + " (" + parseInt(progress * 100.0) + "%)");

    // Status 2 = this part is complete
    if (obj.Status == 2)
    {
        currentPartIndex++;

        if (currentPartIndex == sampleFiles.length)
            Console.print("Installation complete!");
    }
}

inline function startInstallation(sourceDir, targetDir)
{
    installTarget = targetDir;

    // Validate all files exist and match expected sizes
    for (f in sampleFiles)
    {
        local zipFile = sourceDir.getChildFile(f.FileName);

        if (!zipFile.isFile())
        {
            Console.print("Missing file: " + f.FileName);
            return;
        }

        if (zipFile.getSize() != f.FileSize)
        {
            Console.print(f.FileName + " is corrupt");
            return;
        }
    }

    // Start sequential extraction (each call queues on Sample Loading Thread)
    currentPartIndex = 0;

    for (f in sampleFiles)
    {
        local zipFile = sourceDir.getChildFile(f.FileName);
        zipFile.extractZipFile(targetDir, true, onZipProgress);
    }
}

// Trigger installation from user-selected source
FileSystem.browseForDirectory(FileSystem.Downloads, function(sourceDir)
{
    startInstallation(sourceDir, FileSystem.getFolder(FileSystem.Samples));
});
```
```json:testMetadata:multi-part-zip-installation
{
  "testable": false,
  "skipReason": "Requires ZIP files on disk; performs filesystem extraction with side effects"
}
```

**Pitfalls:**
- When extracting multiple ZIP files sequentially, each `extractZipFile` call queues on the Sample Loading Thread via `killVoicesAndCall`. Track a part index to calculate combined progress across all archives.
- Always validate file sizes before extraction. A partially downloaded ZIP will fail silently during extraction. Compare `getSize()` against known expected sizes.
