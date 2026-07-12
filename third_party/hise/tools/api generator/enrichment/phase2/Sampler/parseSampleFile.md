## parseSampleFile

**Examples:**

```javascript:parse-and-import
// Title: Import a user-dropped audio file into the sampler
// Context: Parse an audio file's metadata (loop points, sample length) and
// load it as a single-sample map. The parsed JSON is compatible with loadSampleMapFromJSON().

const var sampler = Synth.getSampler("MainSampler");

inline function importAudioFile(file)
{
    // parseSampleFile reads loop points and sample boundaries from the file header
    // and creates a relative reference if the file is in the Samples folder
    local sampleData = [sampler.parseSampleFile(file)];

    // Wrap in an array - loadSampleMapFromJSON expects an array of sample objects
    sampler.loadSampleMapFromJSON(sampleData);
}

// Browse for a file and import it
FileSystem.browse(FileSystem.Samples, false, "*.wav", importAudioFile);
```

```json:testMetadata:parse-and-import
{
  "testable": false,
  "skipReason": "Requires audio file on disk"
}
```

The returned JSON object contains all standard sample properties (`Root`, `SampleStart`, `SampleEnd`, `LoopStart`, `LoopEnd`, `LoopEnabled`, etc.) extracted from the audio file's metadata. Wrap it in an array before passing to `loadSampleMapFromJSON()`.
