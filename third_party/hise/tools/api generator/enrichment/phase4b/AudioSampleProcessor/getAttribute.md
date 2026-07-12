AudioSampleProcessor::getAttribute(Number parameterIndex) -> Double

Thread safety: SAFE
Returns the current value of the module parameter at the given index. Use the dynamic
constants on the handle (e.g., asp.Gain, asp.SyncMode) as the index.
Required setup:
  const var asp = Synth.getAudioSampleProcessor("AudioLooper1");
Dispatch/mechanics:
  audioSampleProcessor->getAttribute(index) -> Processor::getAttribute()
Pair with:
  setAttribute -- set the value of the same parameter
  getAttributeId -- discover parameter names by index
Source:
  ScriptingApiObjects.cpp:4763+  getAttribute() -> Processor::getAttribute(index)
