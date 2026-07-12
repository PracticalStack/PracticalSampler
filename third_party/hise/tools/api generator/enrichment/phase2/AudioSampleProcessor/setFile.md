## setFile

**Examples:**

```javascript:user-import-file-browser
// Title: Loading a user-imported sample via file browser
// Context: Drum machines and sample players let users import their own audio files.
// The browse callback returns a File object; use toString(0) for the absolute path.

const var player = Synth.getAudioSampleProcessor("AudioLooper1");

inline function onImportButton(component, value)
{
    if (value)
    {
        // Open browser starting from the Samples folder
        FileSystem.browse(FileSystem.getFolder(FileSystem.Samples), false, "*.wav", function(newFile)
        {
            player.setFile(newFile.toString(0));
        });
    }
}

Content.getComponent("ImportButton").setControlCallback(onImportButton);
```
```json:testMetadata:user-import-file-browser
{
  "testable": false,
  "skipReason": "Requires an AudioLooper module, an ImportButton component, and FileSystem.browse user interaction."
}
```

```javascript:ir-selection-combobox
// Title: Loading an IR file from a selection list
// Context: Convolution reverb plugins populate a ComboBox with IR names and load
// the selected file using a pool reference string with {SAMPLE_FOLDER}.

const var reverb = Synth.getAudioSampleProcessor("ConvolutionReverb1");

// Build a list of IR files from the Samples/IR subfolder
const var irFolder = FileSystem.getFolder(FileSystem.Samples).getChildFile("IR");
const var irFiles = FileSystem.findFiles(irFolder, "*.wav", false);

const var irSelector = Content.getComponent("IRSelector");
irSelector.set("items", "");

for (f in irFiles)
    irSelector.addItem(f.toString(1));

inline function onIRSelected(component, value)
{
    if (value > 0)
    {
        local file = irFiles[value - 1];
        reverb.setFile(file.toString(0));
    }
}

irSelector.setControlCallback(onIRSelected);
```
```json:testMetadata:ir-selection-combobox
{
  "testable": false,
  "skipReason": "Requires a ConvolutionReverb module, an IRSelector component, and IR audio files on disk."
}
```

**Pitfalls:**
- When loading files from a `FileSystem.browse()` callback, use `file.toString(0)` (absolute path). Pool reference wildcards like `{PROJECT_FOLDER}` only work for files inside the project's AudioFiles directory.
