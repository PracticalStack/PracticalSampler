Effect (object)
Obtain via: Synth.getEffect("EffectId"), Synth.addEffect(type, id, index), or Builder.create()

Script handle for controlling any audio effect module in the HISE module tree.
Wraps filters, reverbs, delays, dynamics, etc. with a uniform interface for
parameter access, bypass, output metering, modulator chain management, and
state serialization. Each instance exposes the wrapped effect's parameters as
named constants (e.g., fx.Frequency, fx.Gain) for index-free attribute access.

Complexity tiers:
  1. Basic parameter control: Synth.getEffect, setAttribute, setBypassed,
     isBypassed, getAttribute. Sufficient for controlling any built-in effect
     from script.
  2. State serialization: + exportState, restoreState. FX lock, preset
     management, save/load of effect configurations.
  3. Metering and visualization: + getCurrentLevel, isSuspended. Timer-polled
     peak meters and silence detection UI.
  4. Interactive filter display: + setDraggableFilterData, getDraggableFilterData.
     Only for effects implementing ProcessorWithCustomFilterStatistics (Script FX,
     Hardcoded FX, Polyphonic Filter).

Practical defaults:
  - Store all Effect references as const var at the top of onInit. Use arrays
    for repeated types: const var eqs = []; for(i = 0; i < N; i++)
    eqs.push(Synth.getEffect("EQ" + (i+1)));
  - Use named constants (fx.Frequency, fx.Gain) instead of raw integer indices
    for setAttribute/getAttribute. They are generated per-effect-type and
    survive parameter reordering.
  - Prefer setBypassed() over setting an internal enable parameter --
    setBypassed() provides soft bypass with fade-out on MasterEffectProcessors,
    avoiding clicks.
  - Poll getCurrentLevel() at ~30ms intervals in a timer callback with
    exponential decay smoothing (e.g., factor 0.77) for stable peak meters.

Common mistakes:
  - Calling Synth.getEffect() outside onInit -- throws script error. Store
    the reference as a top-level const var.
  - Using exportScriptControls/restoreScriptControls on a built-in effect --
    throws script error. Use exportState/restoreState instead.
  - Using raw integer indices (fx.setAttribute(0, 1000.0)) instead of named
    constants (fx.setAttribute(fx.Frequency, 1000.0)) -- fragile, breaks on
    parameter reordering.
  - Expecting UI to update automatically after restoreState() -- it does not
    send attribute notifications. Re-set parameters or call
    updateValueFromProcessorConnection() on connected components.
  - Polling getCurrentLevel() without smoothing -- raw values fluctuate rapidly,
    producing jittery meters.

Example:
  // Get a reference to an effect in onInit
  const var fx = Synth.getEffect("MyFilter");

  // Use named constants for parameter access
  fx.setAttribute(fx.Frequency, 1000.0);
  fx.setAttribute(fx.Q, 0.7);

Methods (21):
  addGlobalModulator          addModulator
  addStaticGlobalModulator    exists
  exportScriptControls        exportState
  getAttribute                getAttributeId
  getAttributeIndex           getCurrentLevel
  getDraggableFilterData      getId
  getModulatorChain           getNumAttributes
  isBypassed                  isSuspended
  restoreScriptControls       restoreState
  setAttribute                setBypassed
  setDraggableFilterData
