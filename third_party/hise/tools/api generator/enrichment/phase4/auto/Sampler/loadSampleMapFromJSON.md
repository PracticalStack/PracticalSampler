Loads a sample map from a JSON array of sample descriptor objects. Each object must have at least a `FileName` property; other properties (`Root`, `LoKey`, `HiKey`, `LoVel`, `HiVel`, `RRGroup`, etc.) are optional with sensible defaults. Use `File.toReferenceString()` to convert absolute paths to the `{PROJECT_FOLDER}` relative format when the sample file is in your project's Samples folder.

For preset persistence, use `Sampler.getSampleMapAsBase64()` rather than re-saving the original JSON array - the base64 format captures post-load edits such as sample range changes made via an AudioWaveform.

> [!Warning:JSON maps not auto-persisted in presets] The sampler does not store JSON-loaded maps automatically. Your script is responsible for persisting and restoring this data in user presets.
