ScriptAudioWaveform::registerAtParent(Integer index) -> ScriptObject

Thread safety: UNSAFE
Registers the component's internal audio data with the parent script processor at the
given slot index. Returns a ScriptAudioFile handle for programmatic control. Returns
undefined if the parent is not a ProcessorWithDynamicExternalData.

Required setup:
  const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
  const var af = wf.registerAtParent(0);

Dispatch/mechanics:
  registerAtParent(index) -> registerComplexDataObjectAtParent(index)
    -> sets otherHolder to parent processor
    -> calls d->registerExternalObject(AudioFile, index, ownedObject)
    -> returns new ScriptAudioFile(processor, index)

Pair with:
  referToData -- alternative way to connect to external data (inbound vs outbound)

Source:
  ScriptingApiContent.cpp  ScriptAudioWaveform::registerAtParent()
    -> ComplexDataScriptComponent::registerComplexDataObjectAtParent(pIndex)
