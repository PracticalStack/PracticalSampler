AudioSampleProcessor::setBypassed(Number shouldBeBypassed) -> undefined

Thread safety: UNSAFE -- sends a synchronous notification and dispatches a ProcessorChangeEvent.
Sets the bypass state of the wrapped processor module. When bypassed, the module's
processing is skipped.
Dispatch/mechanics:
  audioSampleProcessor->setBypassed(shouldBeBypassed, sendNotification)
    -> sendOtherChangeMessage(ProcessorChangeEvent::Bypassed)
Pair with:
  isBypassed -- query the current bypass state
Source:
  ScriptingApiObjects.cpp:4763+  setBypassed() -> Processor::setBypassed() + sendOtherChangeMessage(Bypassed)
