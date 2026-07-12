Modulator::setBypassed(Number shouldBeBypassed) -> undefined

Thread safety: UNSAFE -- calls setBypassed with sendNotification (synchronous
listener dispatch) and sendOtherChangeMessage for UI update.
Enables or disables the bypass state of the modulator. When bypassed, the
modulator's output is not applied to its target.

Dispatch/mechanics:
  mod->setBypassed(shouldBeBypassed, sendNotification)
    -> mod->sendOtherChangeMessage(ProcessorChangeEvent::Bypassed)
    -> UI notified of bypass state change

Pair with:
  isBypassed -- read back the current bypass state

Source:
  ScriptingApiObjects.cpp  setBypassed()
    -> mod->setBypassed(shouldBeBypassed, sendNotification)
    -> mod->sendOtherChangeMessage(Bypassed)
