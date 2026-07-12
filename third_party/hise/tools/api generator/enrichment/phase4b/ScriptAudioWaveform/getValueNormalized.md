ScriptAudioWaveform::getValueNormalized() -> Double

Thread safety: SAFE
Returns the normalized value (0.0 to 1.0). Base implementation returns
getValue() directly. Only meaningful on ScriptSlider where range mapping
applies.

Pair with:
  setValueNormalized -- to set the normalized value
  getValue -- to read the raw value

Source:
  ScriptingApiContent.cpp  ScriptComponent::getValueNormalized()
    -> returns getValue()
