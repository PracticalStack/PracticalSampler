Reads only the time signature metadata from a MIDI file without loading individual events. Returns a JSON object:

```json
{
  "Nominator": 4.0,
  "Denominator": 4.0,
  "NumBars": 8.0,
  "Tempo": 120.0,
  "LoopStart": 0.0,   // normalised 0.0 - 1.0
  "LoopEnd": 1.0       // normalised 0.0 - 1.0
}
```

This is a lightweight alternative to `loadAsMidiFile` when you only need tempo and time signature information. Returns `undefined` if the file cannot be parsed as MIDI.
