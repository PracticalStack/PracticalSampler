ScriptComboBox::getValueNormalized() -> Double

Thread safety: SAFE
Returns the normalized value (0.0 to 1.0). Base implementation returns
getValue() directly without range mapping. For ScriptComboBox, returns the
raw stored value without converting from the 1..N range.
Source:
  ScriptingApiContent.h  ScriptComponent::getValueNormalized()
    -> returns getValue() directly, no range conversion
