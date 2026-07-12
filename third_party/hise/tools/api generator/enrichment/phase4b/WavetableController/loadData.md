WavetableController::loadData(var bufferOrFile, Number sampleRate, Array loopRange) -> undefined

Thread safety: UNSAFE -- buffer operations, potential file I/O when loading from ScriptFile
Loads audio data into the wavetable synth for resynthesis. Accepts three input
types: ScriptFile (loads from audio file reference), Array of Buffers (multi-channel),
or single Buffer (mono). When loading from ScriptFile, sampleRate and loopRange are ignored.

Required setup:
  const var wc = Synth.getWavetableController("WavetableSynth1");
  wc.setResynthesisOptions(options); // configure before resynthesise

Dispatch/mechanics:
  ScriptFile -> PoolReference -> buffer.fromBase64String(ref)
  Array of Buffers -> extracts float pointers, creates AudioSampleBuffer
  Single Buffer -> loads directly with sampleRate and loopRange

Pair with:
  resynthesise -- call after loading data to trigger wavetable generation
  setResynthesisOptions -- configure analysis parameters before resynthesis

Anti-patterns:
  - Do NOT pass invalid loopRange format -- silently ignored if not an array with
    exactly 2 elements, no error produced
  - Do NOT assume sampleRate/loopRange are used with ScriptFile input -- they are
    silently ignored, the file reference carries its own metadata

Source:
  ScriptingApiObjects.cpp:5321  loadData()
    -> ScriptFile: PoolReference + fromBase64String
    -> Array: extracts Buffer pointers into stack array -> AudioSampleBuffer
    -> Buffer: direct load with sampleRate, loopRange
