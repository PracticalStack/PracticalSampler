Modulator::setIsBipolar(Number shouldBeBipolar) -> undefined

Thread safety: SAFE
Sets whether the modulator operates in bipolar mode. In GainMode, unipolar
output is 0..1 while bipolar output is -1..1. Affects how the modulation
intensity value is applied to the destination signal.

Pair with:
  isBipolar -- read back the current bipolar state

Source:
  ScriptingApiObjects.cpp  setIsBipolar()
    -> dynamic_cast<Modulation*>(mod.get())->setIsBipolar()
