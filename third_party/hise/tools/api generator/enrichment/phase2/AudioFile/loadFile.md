## loadFile

**Examples:**

```javascript:pool-waveform-loading
// Title: Loading embedded waveforms from the audio file pool
// Context: A synth with selectable waveform presets loads audio files
// from the pool into a processor's AudioFile slot via combo box.

// Cache pool references at init
const var WAVEFORM_LIST = Engine.loadAudioFilesIntoPool();

// Obtain the AudioFile handle from the processor
const var asp = Synth.getAudioSampleProcessor("WavetableSynth1");
const var waveformSlot = asp.getAudioFile(0);

// Load a waveform by pool reference (e.g. from a combo box callback)
inline function onWaveformSelect(component, value)
{
    // Pool references are strings like "{PROJECT_FOLDER}saw.wav"
    local poolRef = WAVEFORM_LIST[parseInt(value) - 1];
    waveformSlot.loadFile(poolRef);
}
```
```json:testMetadata:pool-waveform-loading
{
  "testable": false,
  "skipReason": "Requires audio files in the project pool; callback pattern is not self-contained"
}
```

```javascript:user-browsed-file
// Title: Loading a user-browsed audio file
// Context: A file browser lets the user import their own sample into
// a processor's audio slot.

const var playerSlot = Synth.getAudioSampleProcessor("AudioPlayer1");
const var audioFile = playerSlot.getAudioFile(0);

inline function onBrowseButton(component, value)
{
    if(!value)
        return;

    // Browse returns a File object; convert to string for loadFile
    FileSystem.browse(FileSystem.AudioFiles, false, "*.wav", function [audioFile](result)
    {
        audioFile.loadFile(result.toString(0));
    });
}
```
```json:testMetadata:user-browsed-file
{
  "testable": false,
  "skipReason": "FileSystem.browse opens an interactive file dialog requiring user interaction"
}
```

**Pitfalls:**
- When loading from a user file browser, pass `file.toString(0)` (full path string) to `loadFile()`, not the File object directly. The method expects a string argument.
