Synth::deferCallbacks(Integer makeAsynchronous) -> undefined

Thread safety: UNSAFE -- stops synth timer, in deferred transition starts a JUCE Timer which may allocate.
Switches MIDI callback execution between audio thread (default) and message thread (deferred).
When deferred: MIDI messages become read-only, timer switches to JUCE Timer (ms resolution),
audio-thread safety relaxed (allocations/string ops/UI updates safe in callbacks).

Dispatch/mechanics:
  dynamic_cast<JavascriptMidiProcessor*> -> deferCallbacks(makeAsync)
  -> stops any running timer (synth or JUCE depending on transition direction)
  -> deferred mode: callbacks run on message thread, JUCE Timer for onTimer
  -> non-deferred mode: callbacks run on audio thread, synth timer for onTimer

Pair with:
  startTimer / stopTimer -- timer behavior changes with deferred mode
  addToFront -- typically called together in onInit for UI-controller scripts

Anti-patterns:
  - Do NOT switch from deferred to non-deferred and expect the timer to keep running --
    the mode switch stops the timer. Call startTimer() again after switching back.
  - Do NOT call from non-JavascriptMidiProcessor context -- unchecked dynamic_cast
    produces null pointer dereference.

Source:
  ScriptingApi.cpp  Synth::deferCallbacks()
    -> JavascriptMidiProcessor::deferCallbacks(makeAsynchronous)
