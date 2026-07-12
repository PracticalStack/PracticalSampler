## getAudioFileList

**Examples:**

```javascript:expansion-audio-file-list
// Title: Unified audio file selector across all expansions
// Context: Collect audio files from the project root and every installed
// expansion into a single list for a combo box selector.

const var AudioPlayer = Synth.getAudioSampleProcessor("Audio Loop Player1");
const var FileSelector = Content.getComponent("FileSelector");
const var expHandler = Engine.createExpansionHandler();

const var displayNames = ["No file"];
const var poolReferences = [""];

// Collect root project audio files
const var rootFiles = Engine.loadAudioFilesIntoPool();

for (r in rootFiles)
{
    displayNames.push(r.split("}")[1]);
    poolReferences.push(r);
}

// Collect audio files from each expansion
for (e in expHandler.getExpansionList())
{
    for (af in e.getAudioFileList())
    {
        displayNames.push(af.split("}")[1]);
        poolReferences.push(af);
    }
}

FileSelector.set("items", displayNames.join("\n"));

inline function onFileSelectorControl(component, value)
{
    AudioPlayer.setFile(poolReferences[value - 1]);
};

FileSelector.setControlCallback(onFileSelectorControl);
```

```json:testMetadata:expansion-audio-file-list
{
  "testable": false,
  "skipReason": "Requires installed expansion packs with audio files"
}
```
