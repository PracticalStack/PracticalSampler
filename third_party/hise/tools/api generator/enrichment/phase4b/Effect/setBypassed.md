Effect::setBypassed(Number shouldBeBypassed) -> undefined

Thread safety: UNSAFE -- sends bypass notification and an async dispatch message for UI update.
Enables or disables bypass. When bypassed, audio passes through without
processing. MasterEffectProcessor subclasses use soft bypass with fade-out
to avoid clicks.
Dispatch/mechanics:
  effect->setBypassed(shouldBeBypassed, sendNotification)
    -> effect->sendOtherChangeMessage(ProcessorChangeEvent::Bypassed, sendNotificationAsync)
Pair with:
  isBypassed -- query current bypass state
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::setBypassed()
    -> effect->setBypassed() + sendOtherChangeMessage(Bypassed, async)
