ScriptAudioWaveform::getChildComponents() -> Array

Thread safety: UNSAFE
Returns an array of ScriptComponent references for all child components
(those whose parentComponent is set to this component).

Source:
  ScriptingApiContent.cpp  ScriptComponent::getChildComponents()
    -> iterates components with matching parentComponent
