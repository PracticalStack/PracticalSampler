Reads the audio file's header without loading sample data. Returns a JSON object describing the format:

```json
{
  "SampleRate": 44100.0,
  "NumChannels": 2,
  "NumSamples": 132300,
  "BitDepth": 24,
  "Format": "WAV",
  "File": "C:/samples/recording.wav",
  "Metadata": {}  // format-specific tags (BWF fields, ID3, etc.)
}
```

Use this instead of `loadAsAudioFile` when you only need format information - it reads only the header, not the sample data.

> [!Warning:Returns undefined on failure silently] Returns `undefined` silently if the file does not exist or is not a recognised audio format. Unlike `loadAsObject`, no script error is reported on failure. Always check with `isDefined()` before accessing properties.
