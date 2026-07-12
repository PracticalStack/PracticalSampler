ScriptAudioWaveform::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of strings containing all active (non-deactivated) property IDs
for this component, including base and child-class-specific properties.

Pair with:
  get -- to read a property value
  set -- to modify a property value

Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties()
    -> iterates property IDs, skips deactivated ones
