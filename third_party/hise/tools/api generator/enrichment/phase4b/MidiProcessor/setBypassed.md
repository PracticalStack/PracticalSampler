MidiProcessor::setBypassed(bool shouldBeBypassed) -> undefined

Thread safety: UNSAFE -- sends bypass notification and dispatches a ProcessorChangeEvent, which involves listener callbacks.
Sets the bypass state of the MIDI processor. When bypassed, the module does
not process MIDI events.
Dispatch/mechanics:
  mp->setBypassed(shouldBeBypassed, sendNotification)
    -> mp->sendOtherChangeMessage(ProcessorChangeEvent::Bypassed)
Pair with:
  isBypassed -- check current bypass state
Source:
  ScriptingApiObjects.cpp:4679  setBypassed()
    -> mp->setBypassed(shouldBeBypassed, sendNotification)
    -> mp->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Bypassed)
