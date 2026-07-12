Reads a MIDI file and returns a JSON object containing time signature metadata and all MIDI events from the specified track (zero-based index). Only processes files with the `.mid` extension - other file types return an empty result.

```json
{
  "TimeSignature": { ... },  // see loadMidiMetadata for the object format
  "Events": [ ]               // array of MessageHolder objects
}
```

> [!Warning:Fixed sample rate and tempo conversion] Event timestamps are converted using a fixed sample rate of 44100 Hz and tempo of 120 BPM. If playback conditions differ, timestamps may not correspond directly to playback positions.
