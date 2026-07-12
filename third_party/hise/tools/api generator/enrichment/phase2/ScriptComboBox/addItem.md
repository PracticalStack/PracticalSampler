## addItem

**Examples:**

```javascript:populate-from-audio-files
// Title: Populate a combo box from audio files on disk
// Context: Scan a sample folder for audio files and build the item list dynamically.
// Set saveInPreset to false because the file list may differ between machines.

const var cbSelector = Content.addComboBox("FileSelector", 0, 0);
cbSelector.set("saveInPreset", false);

// Clear any stale items left by the Interface Designer
cbSelector.set("items", "");

const var sampleFolder = FileSystem.getFolder(FileSystem.Samples).getChildFile("Loops");
const var audioFiles = FileSystem.findFiles(sampleFolder, "*.wav", false);

for (f in audioFiles)
{
    // Clean up the filename for display
    local name = f.toString(1).replace("_", " ").capitalize();
    cbSelector.addItem(name);
}

inline function onFileSelected(component, value)
{
    if (value > 0)
    {
        local selectedFile = audioFiles[parseInt(value) - 1];
        Console.print("Loading: " + selectedFile.toString(0));
    }
}

cbSelector.setControlCallback(onFileSelected);
```
```json:testMetadata:populate-from-audio-files
{
  "testable": false,
  "skipReason": "Requires audio files in the Samples/Loops folder"
}
```

```javascript:build-from-data-array
// Title: Build items from a data array
// Context: Populate a combo box from an engine-provided list like sample maps or module IDs.

const var mapList = Sampler.getSampleMapList();

const var cbMap = Content.addComboBox("MapSelector", 0, 0);
cbMap.set("saveInPreset", false);

for (m in mapList)
    cbMap.addItem(m);
```
```json:testMetadata:build-from-data-array
{
  "testable": false,
  "skipReason": "Requires sample maps in the project to produce a non-empty list"
}
```
