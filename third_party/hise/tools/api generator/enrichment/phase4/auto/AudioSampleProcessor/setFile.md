Loads an audio file from the pool using a pool reference string. Use `{PROJECT_FOLDER}` for files in the project's AudioFiles folder or `{EXP::expansionName}` for expansion content. When loading user-imported files from a `FileSystem.browse()` callback, pass `file.toString(0)` to get an absolute path rather than a pool wildcard.

> [!Warning:Load pool first in HISE IDE] In the HISE IDE, call `Engine.loadAudioFilesIntoPool()` before using `{PROJECT_FOLDER}` references. Exported plugins embed audio files automatically and do not need this step.

For Convolution Reverb IR loading, access the processor with `Synth.getAudioSampleProcessor("ConvolutionId")`, not `Synth.getEffect()`. The `getEffect()` method returns a generic effect reference that does not expose `setFile()`.

File paths are case-sensitive in compiled plugins even on Windows. Always match the exact filename casing from the AudioFiles folder. The `{PROJECT_FOLDER}` prefix is required for all project-relative references.

Switching files at runtime includes a ~20ms crossfade between the old and new content, so IR changes are click-free.
