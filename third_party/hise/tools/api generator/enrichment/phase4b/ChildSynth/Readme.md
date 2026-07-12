ChildSynth (object)
Obtain via: Synth.getChildSynth(name) or Synth.getChildSynthByIndex(index)

Script handle to a child sound generator module (synth, sampler, group) within
a SynthGroup or SynthChain. Provides attribute control, bypass, modulator chain
access, effect chain reordering, state export/restore, and routing matrix access.

Constants:
  Dynamic (instance-specific, derived from wrapped processor):
    Gain = 0              Volume as gain factor 0..1
    Balance = 1           Stereo balance -100 to 100
    VoiceLimit = 2        Maximum voice count
    KillFadeTime = 3      Fade time when voices are killed
    ScriptParameters      UI component name-to-index mappings (empty if no script interface)

Complexity tiers:
  1. Basic parameter control: setAttribute, getAttribute, setBypassed, isBypassed.
     Controlling child synth parameters and bypass state from the parent script.
  2. Level monitoring and routing: + getCurrentLevel, getRoutingMatrix. VU meters
     and multi-output channel routing.
  3. State management and type casting: + exportState, restoreState, asSampler.
     Preset locking, casting to Sampler for sample map operations.
  4. Dynamic modulation: + addModulator, addGlobalModulator, addStaticGlobalModulator,
     getModulatorChain, setModulationInitialValue. Programmatic modulator chain
     construction for custom modulation matrices.

Practical defaults:
  - Store ChildSynth references in const var arrays when controlling multiple
    channels or oscillators. Build in a loop in onInit.
  - Use dynamic constants (cs.Gain, cs.Balance) rather than raw index numbers
    for setAttribute/getAttribute. Self-documenting and correct across subclasses.
  - Prefer addStaticGlobalModulator over addGlobalModulator when per-voice
    resolution is not needed. One value per block vs per-voice is significantly
    more CPU-efficient.
  - Always check asSampler() return with isDefined() before calling Sampler
    methods. Returns undefined silently for non-sampler types.

Common mistakes:
  - Calling Synth.getChildSynth() outside onInit -- restricted to initialization,
    throws script error at runtime. Store references as const var in onInit.
  - Using chainIndex 0 for addModulator -- index 0 is MidiProcessor, not a
    modulator chain. Use 1 (Gain) or 2 (Pitch).
  - Using addGlobalModulator for all connections -- use addStaticGlobalModulator
    for targets that don't need per-voice resolution (less CPU).
  - Passing getCurrentLevel results directly to UI without smoothing -- raw peaks
    fluctuate rapidly. Apply decay: level = Math.max(newPeak, level * 0.94).
  - Using human-readable type names in addModulator -- must use exact C++ class
    names (e.g., "LFOModulator" not "LFO"). Silent failure on mismatch.

Example:
  // Get a child synth reference in onInit
  const var cs = Synth.getChildSynth("MySynth");

  // Control attributes using dynamic constants
  cs.setAttribute(cs.Gain, 0.5);
  cs.setBypassed(false);

Methods (21):
  addGlobalModulator         addModulator
  addStaticGlobalModulator   asSampler
  exists                     exportState
  getAttribute               getAttributeId
  getAttributeIndex          getChildSynthByIndex
  getCurrentLevel            getId
  getModulatorChain          getNumAttributes
  getRoutingMatrix           isBypassed
  restoreState               setAttribute
  setBypassed                setEffectChainOrder
  setModulationInitialValue
