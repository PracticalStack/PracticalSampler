ChildSynth::setBypassed(bool shouldBeBypassed) -> undefined

Thread safety: UNSAFE -- sends notification and dispatches ProcessorChangeEvent::Bypassed, involves message queue operations
Sets the bypass state of the wrapped synth. When bypassed, the synth does not produce
audio output.
Dispatch/mechanics:
  synth->setBypassed(shouldBeBypassed, sendNotification)
    -> dispatches ProcessorChangeEvent::Bypassed change message
Pair with:
  isBypassed -- check current bypass state
Source:
  ScriptingApiObjects.cpp  setBypassed()
    -> synth->setBypassed(shouldBeBypassed, sendNotification)
