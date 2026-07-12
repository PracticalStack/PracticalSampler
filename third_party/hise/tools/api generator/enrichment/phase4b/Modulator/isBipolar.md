Modulator::isBipolar() -> Integer

Thread safety: SAFE
Returns whether the modulator is in bipolar mode. 1 if bipolar, 0 if unipolar.

Pair with:
  setIsBipolar -- toggle bipolar mode

Source:
  ScriptingApiObjects.cpp  isBipolar()
    -> dynamic_cast<Modulation*>(mod.get())->isBipolar()
