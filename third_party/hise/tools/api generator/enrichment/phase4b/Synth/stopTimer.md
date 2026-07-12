Synth::stopTimer() -> undefined

Thread safety: SAFE -- in non-deferred mode, writes 0.0 to synth timer interval and resets chain index, no allocations. Deferred mode stops a JUCE Timer.
Stops the periodic timer for this script processor. Can be called from within the onTimer
callback itself for one-shot timer behavior.

Dispatch/mechanics:
  Non-deferred (audio thread):
    -> owner->stopSynthTimer(parentMidiProcessor->getIndexInChain())
    -> parentMidiProcessor->setIndexInChain(-1) releases the timer slot
  Deferred (message thread):
    -> owner->stopSynthTimer(jp->getIndexInChain())
    -> jp->stopTimer() stops the JUCE Timer

Pair with:
  startTimer -- start or restart the timer
  isTimerRunning -- check if timer is active
  getTimerInterval -- returns 0.0 after stopTimer

Source:
  ScriptingApi.cpp  Synth::stopTimer()
    -> non-deferred: owner->stopSynthTimer(index) + setIndexInChain(-1)
    -> deferred: owner->stopSynthTimer(index) + jp->stopTimer()
