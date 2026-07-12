Sample::deleteSample() -> undefined

Thread safety: UNSAFE -- schedules async voice-killing via killAllVoicesAndCall, involves lambda allocation and audio thread coordination.
Removes this sample from the sampler's sample map. Deferred -- voices are killed
first, then the sound is removed in an async callback. After deletion, this
Sample object is invalidated.
Dispatch/mechanics:
  killAllVoicesAndCall(lambda) -> lambda removes sound from sample map
  Entire method body guarded by HI_ENABLE_EXPANSION_EDITING preprocessor flag
  In builds without that flag, the method is a silent no-op
Pair with:
  duplicateSample -- clone before deleting if you need the data
  Sampler.createSelection -- rebuild selection after deletions
Anti-patterns:
  - Do NOT use the Sample reference after calling deleteSample() -- any further
    method call throws "Sound does not exist"
  - In builds without HI_ENABLE_EXPANSION_EDITING, this is a silent no-op --
    no error reported but no deletion occurs
Source:
  ScriptingApiObjects.cpp  deleteSample()
    -> killAllVoicesAndCall(lambda)
    -> lambda: removes sound from SampleMap
    -> guarded by #if HI_ENABLE_EXPANSION_EDITING
