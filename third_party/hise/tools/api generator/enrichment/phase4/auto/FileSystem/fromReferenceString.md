Resolves a HISE resource reference string (e.g. `{PROJECT_FOLDER}impulse.wav`) or an absolute file path into a `File` object. The `locationType` parameter determines which resource pool to resolve against and must be one of three constants:

| Location Type | Pool |
|---------------|------|
| `FileSystem.AudioFiles` | Non-streaming audio files (impulse responses, loops) |
| `FileSystem.Samples` | Streaming sample files |
| `FileSystem.UserPresets` | User preset files |

This is useful for converting the reference strings returned by `AudioSampleProcessor.getFilename()` back into `File` objects for display or navigation.

> [!Warning:Returns undefined for embedded resources] Returns `undefined` for embedded resources in exported plugins. Code that works in the HISE IDE - where files exist on disk - will fail silently in compiled plugins if the resource has been embedded into the binary. Check with `isDefined()`.
