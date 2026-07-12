SlotFX::getCurrentEffectId() -> String

Thread safety: WARNING -- string return involves atomic ref-count operations.
Returns the type name string of the currently loaded effect (e.g. "SimpleReverb"),
or "No Effect" if the index is out of range. In DspNetwork mode, returns the
active network's ID string.
Required setup:
  const var slot = Synth.getSlotFX("MyEffectSlot");
Pair with:
  setEffect -- check getCurrentEffectId() before calling setEffect() to avoid
    redundant reloads on preset changes
  getModuleList -- to verify valid effect names
Source:
  SlotFX.h  SlotFX::getCurrentEffectId()
    -> isPositiveAndBelow(currentIndex, effectList.size())
       ? effectList[currentIndex] : "No Effect"
  ScriptingApiObjects.cpp  DspNetwork path: holder->getActiveNetwork()->getId()
