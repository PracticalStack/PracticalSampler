WavetableController (object)
Obtain via: Synth.getWavetableController(processorId)

Script handle for wavetable synth resynthesis, post-FX processing, caching,
and export. Loads audio data from files or buffers, configures resynthesis
options (phase modes, cycle detection, denoising), applies post-FX chains,
and exports results as HWT or WAV files.

Complexity tiers:
  1. Basic wavetable loading: getResynthesisOptions, setResynthesisOptions,
     loadData. Load audio or generated buffers and configure resynthesis.
  2. Wavetable export: + saveAsAudioFile, saveAsHwt. Export processed
     wavetables for reuse or distribution.
  3. Advanced pipeline: + setPostFXProcessors, setEnableResynthesisCache,
     setErrorHandler. Per-cycle post-processing, caching, error reporting.

Practical defaults:
  - Use PhaseMode = "StaticPhase" as default -- preserves natural phase
    relationships without the complexity of dynamic tracking.
  - Set RemoveNoise = false and UseLoris = false when loading single-cycle
    or generated buffers -- noise removal is unnecessary for clean sources.
  - Use a buffer size of 2048 samples at 48000 Hz for generated single-cycle
    waveforms -- good frequency resolution, matches WAV export sample rate.
  - Always set RemoveNoise explicitly due to a deserialization bug where it
    can inherit the value of ReverseOrder.

Common mistakes:
  - Calling resynthesise() without configuring options first -- resynthesis
    uses the current options, configure them before triggering.
  - Enabling noise removal for generated/synthetic waveforms -- adds
    processing time and can alter clean waveforms unnecessarily.
  - Calling Synth.getWavetableController() per operation instead of caching
    the reference -- creates a new wrapper object each time.

Example:
  // Get a reference to a WavetableSynth module
  const var wc = Synth.getWavetableController("WavetableSynth1");

  // Configure resynthesis
  var options = wc.getResynthesisOptions();
  options.PhaseMode = "StaticPhase";
  options.NumCycles = 64;
  wc.setResynthesisOptions(options);

Methods (9):
  getResynthesisOptions       loadData
  resynthesise                saveAsAudioFile
  saveAsHwt                   setEnableResynthesisCache
  setErrorHandler             setPostFXProcessors
  setResynthesisOptions
