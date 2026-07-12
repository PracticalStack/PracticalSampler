Synth::isTimerRunning() -> Integer

Thread safety: SAFE -- reads stored timer interval (non-deferred) or JUCE Timer state (deferred), no allocations.
Returns true if the timer for this script processor is currently running. Non-deferred checks
synth timer interval != 0. Deferred checks JUCE Timer::isTimerRunning().

Source:
  ScriptingApi.cpp  Synth::isTimerRunning()
    -> non-deferred: owner->getTimerInterval(parentMidiProcessor->getIndexInChain()) != 0.0
    -> deferred: jp->isTimerRunning()
