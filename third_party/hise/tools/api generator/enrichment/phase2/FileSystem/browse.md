## browse

**Examples:**

```javascript:import-audio-smart-start-folder
// Title: Import a user audio file with smart start folder
// Context: When browsing for audio content, the dialog should open at the
// most useful location. If a file is already loaded, start at its parent
// directory. Otherwise, fall back to the Samples folder.

const var player = Synth.getAudioSampleProcessor("AudioPlayer1");

inline function onImportButtonClicked(component, value)
{
    if (!value)
        return;

    // Start at the current file's directory if one is loaded
    local startFolder = FileSystem.Samples;
    local currentFilename = player.getFilename();

    if (currentFilename.length)
    {
        local currentFile = FileSystem.fromReferenceString(currentFilename, FileSystem.AudioFiles);

        if (isDefined(currentFile))
            startFolder = currentFile.getParentDirectory();
    }

    FileSystem.browse(startFolder, false, "*.wav,*.aif", function(newFile)
    {
        player.setFile(newFile.toString(0));
    });
};

Content.getComponent("ImportButton").setControlCallback(onImportButtonClicked);
```
```json:testMetadata:import-audio-smart-start-folder
{
  "testable": false,
  "skipReason": "Requires native OS file dialog interaction that cannot be automated. Also depends on AudioSampleProcessor and UI components."
}
```

```javascript:save-load-json-config
// Title: Save and load custom JSON configuration files
// Context: A plugin stores custom configurations (MIDI mappings, mixer
// settings) as JSON files. browse() is used for both save and load
// operations, with forSaving controlling the dialog type.

var currentConfig = {"midiChannel": 1, "volume": 0.8};

inline function applyConfig(data)
{
    currentConfig = data;
    Console.print("Config loaded: " + trace(data));
}

inline function onSaveConfig(component, value)
{
    if (!value) return;

    FileSystem.browse(FileSystem.AppData, true, "*.json", function(f)
    {
        f.writeObject(currentConfig);
    });
};

inline function onLoadConfig(component, value)
{
    if (!value) return;

    FileSystem.browse(FileSystem.AppData, false, "*.json", function(f)
    {
        local loaded = f.loadAsObject();

        if (isDefined(loaded))
            applyConfig(loaded);
    });
};
```
```json:testMetadata:save-load-json-config
{
  "testable": false,
  "skipReason": "Requires native OS file dialog interaction that cannot be automated."
}
```

**Pitfalls:**
- Only one file dialog can be open at a time across all browse methods. If the user rapidly clicks buttons that trigger different browse calls, the second call is silently dropped. Disable the trigger button after opening a dialog and re-enable it in the callback (or after a short timeout for the cancel case).
