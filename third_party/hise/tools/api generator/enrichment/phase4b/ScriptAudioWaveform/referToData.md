ScriptAudioWaveform::referToData(ScriptObject audioData) -> undefined

Thread safety: UNSAFE
Connects the waveform to an external audio data source. Accepts three argument types:
a ScriptAudioFile, another ScriptAudioWaveform (shares same data), or -1 to reset
to the component's own internal buffer.

Required setup:
  const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
  const var af = Engine.createAndRegisterAudioFile(0);
  wf.referToData(af);

Dispatch/mechanics:
  referToData() -> referToDataBase()
    -> ScriptAudioFile: sets otherHolder to the audio file's ExternalDataHolder
    -> ScriptAudioWaveform: sets otherHolder to the other component
    -> -1: clears otherHolder, reverts to internal buffer
    -> updateCachedObjectReference() removes old listener, adds new

Pair with:
  registerAtParent -- reverse direction (registers internal data with parent processor)

Anti-patterns:
  - Passing an object that is neither ScriptAudioFile, ScriptAudioWaveform, nor -1
    silently does nothing -- no error is reported

Source:
  ScriptingApiContent.cpp  ScriptAudioWaveform::referToData()
    -> ComplexDataScriptComponent::referToDataBase(audioData)
    -> dynamic_cast routing to ScriptComplexDataReferenceBase / ComplexDataScriptComponent / int(-1)
