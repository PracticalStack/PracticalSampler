## saveAsAudioFile

**Examples:**

```javascript:export-wav-via-browser
// Title: Export wavetable as WAV via file browser
// Context: User-triggered wavetable export through a save dialog,
//          producing a 48kHz/24-bit WAV with loop point metadata

const var wc = Synth.getWavetableController("WavetableSynth1");

// Open a save dialog and export on selection
FileSystem.browse(FileSystem.AudioFiles, true, "*.wav", function[wc](result)
{
    wc.saveAsAudioFile(result);
});
```

```json:testMetadata:export-wav-via-browser
{
  "testable": false,
  "skipReason": "Requires a WavetableSynth module and opens a file browser dialog"
}
```
