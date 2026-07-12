AudioSampleProcessor (object)
Obtain via: Synth.getAudioSampleProcessor(processorId)

Script handle to any processor module with an audio file slot (Audio Loop Player,
Convolution Reverb, Wavetable Synthesiser, Noise Grain Player). Provides file loading,
sample range and loop range control, and access to the underlying AudioFile complex data object.

Constants:
  (Dynamic -- parameter names added per-instance from the wrapped processor.
   Use getNumAttributes()/getAttributeId(index) to discover at runtime.)

Complexity tiers:
  1. Basic IR/sample loading: setFile, getFilename. Load audio files and query the current
     file. Covers convolution reverb IR selection and simple audio loop playback.
  2. Batch operations with arrays: + getAttribute, setAttribute, getSampleRange,
     setSampleRange. Multi-pad or multi-layer instruments with preset serialization.
  3. AudioFile bridge and change detection: + getAudioFile. Access complex data API for
     broadcaster attachment, file drop targets, and programmatic buffer manipulation.

Practical defaults:
  - Use {PROJECT_FOLDER} references for audio files bundled with the plugin.
    Use absolute paths (from File.toString(0)) for user-imported files.
  - Use getFilename().length as a quick check for whether a file is loaded --
    empty string means no file.
  - Convert pool references to File objects for directory browsing:
    FileSystem.fromReferenceString(asp.getFilename(), FileSystem.AudioFiles).

Complex data chain:

![Audio File Data Chain](topology_complex-audio-data-chain.svg)

  - AudioSampleProcessor selects the module that owns one or more audio file slots.
  - AudioFile is the complex-data handle for one slot within that module.
  - ScriptAudioWaveform displays or edits one selected slot in the UI.

  Use the binding properties separately:
  - processorId selects the owning processor.
  - sampleIndex selects which audio slot inside that processor should be displayed.

  This is not the normal parameter binding path. parameterId targets processor
  parameters, while audio-slot binding uses sampleIndex instead.

Common mistakes:
  - Using a filesystem path with setFile() instead of a pool reference string --
    must use {PROJECT_FOLDER}file.wav or {EXP::name}file.wav format.
  - Calling setFile() in HISE IDE without calling Engine.loadAudioFilesIntoPool()
    first -- required for {PROJECT_FOLDER} references in backend only.
  - Using getFilename() directly as a filesystem path -- returns a pool reference
    string, not a path. Convert via FileSystem.fromReferenceString().
  - Using {PROJECT_FOLDER} for user-imported files from arbitrary disk locations --
    use file.toString(0) (absolute path) instead.

Example:
  // Get a reference to an Audio Loop Player module
  const var asp = Synth.getAudioSampleProcessor("AudioLooper1");

  // Load an audio file from the project pool
  asp.setFile("{PROJECT_FOLDER}my_loop.wav");

  // Set the playback range (in samples)
  asp.setSampleRange(0, asp.getTotalLengthInSamples());

Methods (17):
  exists                getAttribute         getAttributeId
  getAttributeIndex     getAudioFile         getFilename
  getLoopRange          getNumAttributes     getSampleLength
  getSampleRange        getSampleStart       getTotalLengthInSamples
  isBypassed            setAttribute         setBypassed
  setFile               setSampleRange
