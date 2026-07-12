Synth::getTimerInterval() -> Double

Thread safety: SAFE -- reads a stored double (non-deferred) or JUCE Timer interval (deferred), no allocations.
Returns the current timer interval in seconds. Returns 0.0 if no timer is running. Behavior
differs based on deferred mode: non-deferred reads synth timer interval, deferred reads JUCE Timer.

Source:
  ScriptingApi.cpp  Synth::getTimerInterval()
    -> non-deferred: owner->getTimerInterval(parentMidiProcessor->getIndexInChain())
    -> deferred: jp->getTimerInterval() / 1000.0
