AudioSampleProcessor::isBypassed() -> Integer

Thread safety: SAFE
Returns whether the wrapped processor module is currently bypassed.
Returns false if the handle is invalid.
Pair with:
  setBypassed -- toggle bypass state
Source:
  ScriptingApiObjects.cpp:4763+  isBypassed() -> audioSampleProcessor->isBypassed()
