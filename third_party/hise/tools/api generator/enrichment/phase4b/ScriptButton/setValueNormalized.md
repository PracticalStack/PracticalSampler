ScriptButton::setValueNormalized(Double normalizedValue) -> undefined

Thread safety: SAFE
Sets the value using a normalized 0..1 range. For ScriptButton this is equivalent
to setValue() since the button's range is already 0..1. Pass 0.0 for off or 1.0
for on.

Source:
  ScriptingApiContent.cpp  ScriptComponent::setValueNormalized()
