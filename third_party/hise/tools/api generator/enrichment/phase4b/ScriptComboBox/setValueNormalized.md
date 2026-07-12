ScriptComboBox::setValueNormalized(Double normalizedValue) -> undefined

Thread safety: SAFE
Sets the value using a normalized 0..1 range. Base implementation calls
setValue(normalizedValue) directly without range mapping. For ScriptComboBox,
this stores the raw normalized value as-is, which is not useful for integer-indexed
combo boxes.
Anti-patterns:
  - Do NOT use setValueNormalized to select combo box items -- it stores the raw
    float without mapping to the 1..N item range. Use setValue() with an explicit
    integer index instead.
Source:
  ScriptingApiContent.h  ScriptComponent::setValueNormalized()
    -> calls setValue(normalizedValue) directly, no range conversion
