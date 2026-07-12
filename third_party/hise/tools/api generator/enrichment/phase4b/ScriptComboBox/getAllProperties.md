ScriptComboBox::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of strings containing all active (non-deactivated) property IDs.
Includes base ScriptComponent properties and ScriptComboBox-specific properties
(items, fontName, fontSize, fontStyle, enableMidiLearn, popupAlignment,
useCustomPopup). The min and max properties are deactivated on ScriptComboBox
and will not appear.
Source:
  ScriptingApiContent.h  ScriptComponent::getAllProperties()
