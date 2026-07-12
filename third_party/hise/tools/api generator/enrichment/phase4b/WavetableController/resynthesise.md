WavetableController::resynthesise() -> undefined

Thread safety: UNSAFE -- triggers heavy FFT-based resynthesis and SiTraNo decomposition on the sample loading thread
Triggers resynthesis of the currently loaded audio data using the current
resynthesis options. Delegates to reloadWavetable() which performs FFT analysis,
optional Loris decomposition, and SiTraNo noise separation.

Required setup:
  const var wc = Synth.getWavetableController("WavetableSynth1");
  wc.loadData(audioBuffer, 44100.0, [0, 44100]);
  wc.setResynthesisOptions(options);

Dispatch/mechanics:
  reloadWavetable() -> loadWavetableFromIndex(currentBankIndex)
    -> FFT analysis + optional Loris + SiTraNo on sample loading thread
    -> checks resynthesis cache first if enabled

Pair with:
  loadData -- must provide audio source before resynthesising
  setResynthesisOptions -- configure analysis parameters first
  setEnableResynthesisCache -- enable caching to skip redundant resynthesis
  setErrorHandler -- receive error messages from the resynthesis process

Source:
  ScriptingApiObjects.cpp:5247  resynthesise()
    -> wt->reloadWavetable() -> loadWavetableFromIndex()
