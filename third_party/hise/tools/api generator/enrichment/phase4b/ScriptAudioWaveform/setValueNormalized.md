ScriptAudioWaveform::setValueNormalized(Double normalizedValue) -> undefined

Thread safety: SAFE
Sets the value using a normalized 0..1 range. Base implementation calls
setValue(normalizedValue) directly. Only meaningful on ScriptSlider where
range mapping applies.

Pair with:
  getValueNormalized -- to read the normalized value
  setValue -- to set the raw value

Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueNormalized()
    -> calls setValue(normalizedValue)
