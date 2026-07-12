Modulator (object)
Obtain via: Synth.getModulator(name)

Script handle for controlling modulator modules (LFOs, envelopes, constants,
modulator chains) in the HISE module tree. Provides intensity/bypass control,
attribute access by index or name, global modulator connections, child chain
manipulation, and state serialization.

Constants:
  Dynamic -- populated from the wrapped modulator's parameter list at
  construction. Each parameter name maps to its index (e.g., an LFO exposes
  Frequency, FadeIn, TempoSync; an AHDSR exposes Attack, Decay, etc.).
  ScriptParameters object is added when the modulator is a script processor.

Complexity tiers:
  1. Basic reference and control: setAttribute, setIntensity, setBypassed.
     Controlling existing modulators from UI callbacks -- filter toggles, LFO
     rate changes, envelope parameters.
  2. State management: + exportState, restoreState. Preset locking, FX
     snapshots, custom preset systems that capture and restore module state.
  3. Dynamic modulation routing: + addGlobalModulator, addStaticGlobalModulator,
     addModulator, getModulatorChain. Runtime-configurable modulation matrices
     with connections created/removed on demand.
  4. MatrixModulator integration: + setMatrixProperties, setIsBipolar.
     Configuring modulators for the built-in ModulationMatrix system with range
     data and bipolar mode.

Practical defaults:
  - Use const var references at onInit scope for all modulator handles.
    Synth.getModulator is restricted to onInit.
  - Use setIntensity(12.0) for pitch modulators when you want full-range
    semitone control. Pitch mode presents values in semitones internally.
  - Use addStaticGlobalModulator instead of addGlobalModulator for voice-start
    sources (velocity, random, note-number). Static routing is more CPU-efficient.
  - A 30ms timer interval is a good default for getCurrentLevel display polling.
  - When creating per-voice envelope modulators at runtime via addModulator,
    set EcoMode to reduce CPU. A value of 32 balances accuracy and performance.

Common mistakes:
  - Calling Synth.getModulator() outside onInit -- restricted to initialization.
    Store references as top-level const variables.
  - Setting setIntensity(1.0) on a pitch modulator expecting 1 semitone --
    PitchMode intensity IS in semitones (-12 to 12), so 1.0 gives 1 semitone.
    Use 12.0 for full range.
  - Using connectToGlobalModulator on a regular modulator -- only works on
    global receiver types (GlobalTimeVariantModulator, GlobalVoiceStartModulator).
  - Calling exportScriptControls on a non-script modulator -- only works on
    Script Voice Start / Script Time Variant / Script Envelope modulators.
  - Creating duplicate modulation connections without tracking -- each
    addGlobalModulator call adds another modulator to the chain, stacking depth.
  - Using addGlobalModulator for velocity/note-number sources -- use
    addStaticGlobalModulator for voice-start sources that never change mid-note.
  - Calling getCurrentLevel from onControl or onNoteOn -- use a timer callback
    instead. Display value updates once per audio buffer.

Example:
  // Get a reference to an LFO in the module tree
  const var mod = Synth.getModulator("LFO1");

  // Set attributes using the dynamic parameter constants
  mod.setAttribute(mod.Frequency, 2.5);
  mod.setIntensity(0.8);

Methods (27):
  addGlobalModulator         addModulator
  addStaticGlobalModulator   asTableProcessor
  connectToGlobalModulator   exists
  exportScriptControls       exportState
  getAttribute               getAttributeId
  getAttributeIndex          getCurrentLevel
  getGlobalModulatorId       getId
  getIntensity               getModulatorChain
  getNumAttributes           getType
  isBipolar                  isBypassed
  restoreScriptControls      restoreState
  setAttribute               setBypassed
  setIntensity               setIsBipolar
  setMatrixProperties
