SlotFX (object)
Obtain via: Synth.getSlotFX("SlotId")

Dynamic effect slot for runtime loading, swapping, and querying of effect
modules. Wraps either a HotswappableProcessor (classic SlotFX, HardcodedMasterFX)
or a DspNetwork::Holder (scriptnode network loading). Returns Effect handles in
classic mode or DspNetwork objects in scriptnode mode.

Complexity tiers:
  1. Single slot: setEffect, getCurrentEffect. Load one effect and control it
     via the returned Effect handle.
  2. FX rack with presets: + clear, getCurrentEffectId, getModuleList. Multiple
     SlotFX modules managed as an array, driven by ComboBox or popup menu with
     a data-driven config object.
  3. Builder + compiled networks: + getParameterProperties. Programmatic module
     tree construction with HardcodedMasterFX and compiled DSP networks via
     Builder.get(module, b.InterfaceTypes.SlotFX).setEffect("networkName").

Practical defaults:
  - Use setEffect("EmptyFX") or clear() to reset a slot to passthrough. setEffect("EmptyFX")
    is more common because it fits naturally into switch/case selection logic.
  - Always use the Effect handle returned by setEffect() to control the loaded effect.
    Do not look up the child by internal name (e.g., Synth.getEffect("SlotId_EffectType")).
  - Store SlotFX references in arrays when managing multiple slots. Use indexed module
    names ("EffectSlot" + (i + 1)) to initialize them in a loop.
  - Define effect configurations as a data object mapping each effect to its moduleID,
    parameterIds, ranges, and defaultState. Keeps slot management code generic.

Common mistakes:
  - Calling slot.setBypassed(true) -- setBypassed is declared but not registered.
    Use slot.getCurrentEffect().setBypassed(true) or the Effect handle from setEffect().
  - Loading polyphonic effects (PolyFilterEffect, PolyshapeFX) -- these are excluded
    by the SlotFX constrainer. Only monophonic/master effect types can be loaded.
  - Looking up the loaded effect by internal child name (Synth.getEffect("SlotId_EffectType"))
    instead of using the Effect handle returned by setEffect(). The naming is an
    implementation detail.
  - Calling setEffect() repeatedly with the same name on preset load -- the C++ layer
    already skips reloading if the same type is loaded, but checking getCurrentEffectId()
    first avoids unnecessary audio thread coordination.

Example:
  // Get a reference to a SlotFX module in onInit
  const var slot = Synth.getSlotFX("MyEffectSlot");

  // Load a reverb into the slot
  const var fx = slot.setEffect("SimpleReverb");

  // Control the loaded effect
  fx.setAttribute(fx.Size, 0.8);

Methods (8):
  clear                  exists
  getCurrentEffect       getCurrentEffectId
  getModuleList          getParameterProperties
  setEffect              swap
