Content::callAfterDelay(int milliSeconds, Function function, var thisObject) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder (heap allocation) and schedules a JUCE Timer callback on the message thread.
Schedules a function to execute after the specified delay in milliseconds. The callback
runs on the message thread via JUCE Timer -- not sample-accurate, do not use for DSP timing.
Optional third argument sets the `this` context for the callback.
Callback signature: function()

Dispatch/mechanics:
  Creates WeakCallbackHolder -> Timer::callAfterDelay(ms)
  Callback fires on message thread (not audio thread)
  Third arg (thisObject) is optional, defaults to empty var

Anti-patterns:
  - Do NOT use for musical timing -- JUCE Timer is not sample-accurate. Use
    TransportHandler grid callbacks or Synth.addTimer for DSP-precise scheduling.

Source:
  ScriptingApiContent.cpp:9058  Content::callAfterDelay()
    -> WeakCallbackHolder construction
    -> Timer::callAfterDelay() (JUCE timer system)
