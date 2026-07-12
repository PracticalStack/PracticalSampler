Sampler::importSamples(Array fileNameList, Integer skipExistingSamples) -> Array

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, kills voices, acquires SampleLock, file I/O
Imports audio files into the sampler and returns an array of Sample objects.
Only available in backend/expansion editing builds (HI_ENABLE_EXPANSION_EDITING).
Auto-assigns root notes starting from note 0.
Anti-patterns:
  - Do NOT rely on this in exported plugins -- returns empty array silently
    (HI_ENABLE_EXPANSION_EDITING is disabled in frontend builds)
Dispatch/mechanics:
  Kills voices synchronously -> acquires SampleLock
    -> SampleImporter::loadAudioFilesUsingDropPoint() with auto root assignment
    -> returns ScriptingSamplerSound array for newly imported samples
Pair with:
  parseSampleFile -- parse metadata before import
  loadSampleMapFromJSON -- alternative: build map from JSON descriptors
Source:
  ScriptingApi.cpp  Sampler::importSamples()
    -> guarded by #if HI_ENABLE_EXPANSION_EDITING
    -> SampleImporter::loadAudioFilesUsingDropPoint()
