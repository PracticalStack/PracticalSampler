SlotFX::getModuleList() -> Array

Thread safety: UNSAFE -- allocates an Array and String elements.
Returns an array of effect type name strings that can be loaded into this slot.
In HotswappableProcessor mode, returns the filtered list of allowed MasterEffectProcessor
types (excludes polyphonic effects, routing effects, harmonic filters, nested SlotFX).
In DspNetwork mode, returns available scriptnode network XML files (HISE IDE only).
Required setup:
  const var slot = Synth.getSlotFX("MyEffectSlot");
Dispatch/mechanics:
  HotswappableProcessor: slot->getModuleList() -> returns effectList (built at
    construction via EffectProcessorChainFactoryType with Constrainer)
  DspNetwork::Holder (USE_BACKEND only): BackendDllManager::getNetworkFiles()
    -> returns filenames without extension
Pair with:
  setEffect -- use names from getModuleList() as the effectName parameter
Anti-patterns:
  - Do NOT rely on getModuleList() in DspNetwork mode for exported plugins --
    silently returns an empty array with no error. Only works in HISE IDE
    (USE_BACKEND builds).
Source:
  ScriptingApiObjects.cpp:3804  ScriptingSlotFX::getModuleList()
    -> getSlotFX()->getModuleList() (HotswappableProcessor path)
    -> or BackendDllManager::getNetworkFiles() under #if USE_BACKEND
