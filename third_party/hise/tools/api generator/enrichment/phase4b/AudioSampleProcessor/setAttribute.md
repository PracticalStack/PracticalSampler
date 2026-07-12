AudioSampleProcessor::setAttribute(Number parameterIndex, Number newValue) -> undefined

Thread safety: UNSAFE -- sends an asynchronous notification that triggers ValueTree property changes and listener callbacks.
Sets the value of the module parameter at the given index. Use the dynamic constants on
the handle (e.g., asp.Gain, asp.SyncMode) as the index.
Required setup:
  const var asp = Synth.getAudioSampleProcessor("AudioLooper1");
Dispatch/mechanics:
  audioSampleProcessor->setAttribute(index, value, sendNotificationAsync)
    -> updates parameter + fires async ValueTree change notification
Pair with:
  getAttribute -- read the current value
  getAttributeId -- discover parameter names by index
Source:
  ScriptingApiObjects.cpp:4763+  setAttribute() -> Processor::setAttribute(index, value, sendNotificationAsync)
