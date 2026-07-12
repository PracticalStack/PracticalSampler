AudioSampleProcessor::setFile(String fileName) -> undefined

Thread safety: UNSAFE -- loads audio data from the pool, involves I/O and complex processing.
Loads an audio file from the HISE audio file pool. The fileName is a pool reference string,
not a filesystem path. Use {PROJECT_FOLDER}file.wav for project files, {EXP::name}file.wav
for expansion files.
Required setup:
  const var asp = Synth.getAudioSampleProcessor("AudioLooper1");
  // In HISE IDE only:
  Engine.loadAudioFilesIntoPool();
Dispatch/mechanics:
  #if USE_BACKEND: validates pool is loaded for {PROJECT_FOLDER} refs (skipped for {EXP::})
  ProcessorWithExternalData->getAudioFile(0)->fromBase64String(fileName)
    -> MultiChannelAudioBuffer loads from pool
Pair with:
  getFilename -- query the currently loaded file reference
  getAudioFile -- access the AudioFile complex data object after loading
Anti-patterns:
  - Do NOT pass a filesystem path (e.g., "C:/audio/file.wav") -- must use pool reference
    format with {PROJECT_FOLDER} or {EXP::name} wildcards.
  - Do NOT call in HISE IDE without Engine.loadAudioFilesIntoPool() first -- reports
    script error for {PROJECT_FOLDER} refs. Not needed in exported plugins.
  - Do NOT use Synth.getEffect() to access setFile() -- use Synth.getAudioSampleProcessor("id").
Convolution Reverb notes:
  File paths are case-sensitive in compiled plugins. Match exact casing.
  IR switches include a ~20ms crossfade (click-free).
Source:
  ScriptingApiObjects.cpp:4887  setFile()
    -> #if USE_BACKEND: pool loaded check + {EXP::} bypass
    -> getAudioFile(0)->fromBase64String(fileName)
