Synth::startTimer(Double seconds) -> undefined

Thread safety: SAFE -- in non-deferred mode, writes to atomic timer interval storage, no allocations, no locks. Deferred mode starts a JUCE Timer (may allocate internally).
Starts or restarts the periodic timer for this script processor. The onTimer callback fires
repeatedly at the specified interval. Minimum interval is 0.004 seconds (4ms).

Dispatch/mechanics:
  Non-deferred (audio thread, default):
    -> owner->startSynthTimer(parentMidiProcessor->getIndexInChain(), interval, timestamp)
    -> allocates one of 4 timer slots per synth; reuses existing slot if already running
    -> timer events inserted as HiseEvent::TimerEvent, rastered to HISE_EVENT_RASTER
  Deferred (message thread):
    -> stops any running synth timer slot first
    -> jp->startTimer((int)(seconds * 1000)) via JUCE Timer
    -> no sample-accurate timing, no slot limit

Pair with:
  stopTimer -- stop the timer, release the slot
  isTimerRunning -- check if timer is active
  getTimerInterval -- read current interval
  deferCallbacks -- switches between audio-thread and message-thread timer modes

Anti-patterns:
  - Do NOT start timers from more than 4 script processors in the same synth (non-deferred) --
    only 4 timer slots exist per synth. The 5th call fails with "All 4 timers are used".
  - Do NOT use intervals below 0.004 seconds -- produces script error "Go easy on the timer!".
  - Do NOT call from a modulator or effect script -- produces "Timers only work in MIDI
    processors!". Requires a MIDI processor context.

Source:
  ScriptingApi.cpp  Synth::startTimer()
    -> non-deferred: owner->startSynthTimer(index, interval, timestamp)
    -> deferred: jp->startTimer((int)(seconds * 1000))
