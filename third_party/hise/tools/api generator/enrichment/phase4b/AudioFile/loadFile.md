AudioFile::loadFile(String filePath) -> undefined

Thread safety: UNSAFE -- file I/O, acquires write lock on the audio buffer.
Loads an audio file from a HISE pool reference string. Pass an empty string to
clear the buffer. Loading is synchronous with a write lock on the underlying buffer.

Required setup:
  const var asp = Synth.getAudioSampleProcessor("AudioLoopPlayer1");
  const var af = asp.getAudioFile(0);

Dispatch/mechanics:
  buffer->fromBase64String(filePath)
    -> empty string: clears buffer, sends content redirect
    -> standard ref: DataProvider resolves reference -> sets originalBuffer,
       bufferRange, sampleRate, loopRange -> sends content redirect

Pair with:
  getCurrentlyLoadedFile -- retrieve the reference string of the loaded file
  Engine.loadAudioFilesIntoPool -- get valid pool reference strings at init

Anti-patterns:
  - Do NOT pass a filesystem path ("C:/audio/file.wav") -- expects a HISE pool
    reference ("{PROJECT_FOLDER}file.wav") or output of File.toString(0).

Source:
  ScriptingApiObjects.cpp  ScriptAudioFile::loadFile()
    -> buffer->fromBase64String(filePath)
    -> SampleDisplayComponent.cpp:2688  MultiChannelAudioBuffer::fromBase64String()
    -> acquires SimpleReadWriteLock::ScopedWriteLock
    -> DataProvider::loadFile() -> SampleReference with buffer, sampleRate, loopRange
