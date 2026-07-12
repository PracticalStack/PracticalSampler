WavetableController::saveAsHwt(ScriptObject outputFile) -> undefined

Thread safety: UNSAFE -- file I/O operations (delete, create, write binary stream)
Saves the currently loaded wavetable data as an HWT file (HISE Wavetable binary
format). The HWT format stores the wavetable as a binary ValueTree that can be
loaded back into a WavetableSynth.

Pair with:
  resynthesise -- ensure wavetable data exists before saving
  saveAsAudioFile -- alternative export format (48kHz WAV with loop points)

Anti-patterns:
  - Do NOT pass a string path -- must be a ScriptFile object (unlike saveAsAudioFile
    which accepts both). Passing other types silently does nothing.
  - Do NOT call without loaded wavetable data -- silently does nothing if no
    wavetable is currently loaded, no error produced

Source:
  ScriptingApiObjects.cpp:5255  saveAsHwt()
    -> getCurrentlyLoadedWavetableTree() -> ValueTree
    -> dynamic_cast<ScriptFile*> check
    -> v.writeToStream(fos)
