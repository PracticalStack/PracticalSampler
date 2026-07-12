WavetableController::saveAsAudioFile(ScriptObject outputFile) -> undefined

Thread safety: UNSAFE -- file I/O operations (delete, create, write WAV)
Saves the currently loaded wavetable as a WAV audio file at 48000 Hz, 24-bit.
Output includes loop metadata (Loop0Start=0, Loop0End=tableSize-1, NumSampleLoops=1)
so the file can be reimported with correct loop points.

Pair with:
  resynthesise -- ensure wavetable data exists before saving
  saveAsHwt -- alternative export format (binary ValueTree)

Anti-patterns:
  - Do NOT assume an error is thrown when no wavetable synth is connected --
    this method silently returns on invalid synth reference (unlike all other
    methods in this class)

Source:
  ScriptingApiObjects.cpp:5278  saveAsAudioFile()
    -> createAudioSampleBufferFromWavetable(0) -> getTableSize()
    -> writes WAV at 48000 Hz, 24-bit with loop metadata
    -> file resolved via FileSystem::getFileFromVar
