Modulator::setIntensity(Number newIntensity) -> undefined

Thread safety: SAFE -- writes to a lock-free LinearSmoothedValue and dispatches
notification asynchronously.
Sets the modulation intensity. Valid range depends on the parent chain's mode:
GainMode: 0.0..1.0, PitchMode: -12.0..12.0 (semitones), Pan/Global/Offset/
Combined: -1.0..1.0. Values outside range are clamped without error.

Dispatch/mechanics:
  GainMode: clamp [0.0, 1.0] -> m->setIntensity(value)
  PitchMode: clamp [-12.0, 12.0] -> m->setIntensity(value / 12.0)
  else: clamp [-1.0, 1.0] -> m->setIntensity(value)
    -> writes LinearSmoothedValue<float>
    -> sendOtherChangeMessage(Intensity, sendNotificationAsync)

Pair with:
  getIntensity -- read back the current intensity value

Anti-patterns:
  - Do NOT assume PitchMode intensity is 0..1 normalized -- it is in semitones
    (-12 to 12). setIntensity(1.0) gives only 1 semitone of range.

Source:
  ScriptingApiObjects.cpp:3121  setIntensity()
    -> branches on Modulation::Mode
    -> m->setIntensity(value) with mode-dependent clamping/conversion
