Converts this file's absolute path to a HISE pool reference string (e.g. `{PROJECT_FOLDER}sound.wav`) relative to the specified project subdirectory. This is useful when importing custom samples into a samplemap - passing the reference string to `Sampler.loadSampleMapFromJSON` ensures the path is portable across machines.

The `folderType` parameter accepts these identifiers:

| Folder Type | Description |
|-------------|-------------|
| `"AudioFiles"` | Non-streaming audio resources |
| `"Images"` | Image resources |
| `"SampleMaps"` | Sample map XML definitions |
| `"MidiFiles"` | MIDI file resources |
| `"UserPresets"` | User presets |
| `"Samples"` | Streaming sample data |
| `"Scripts"` | HiseScript source files |
| `"Binaries"` | Compiled binary output |
| `"Presets"` | Module preset definitions |
| `"XmlPresetBackups"` | XML preset backups |
| `"AdditionalSourceCode"` | Additional C++ source code |
| `"Documentation"` | Documentation files |

> [!Warning:No validation of file location] The method constructs the reference string regardless of whether the file actually resides inside the specified subdirectory. If the file is outside the folder, the returned string will not resolve correctly when used with `FileSystem.fromReferenceString`.
