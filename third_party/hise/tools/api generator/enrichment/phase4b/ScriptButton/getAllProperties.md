ScriptButton::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of strings containing all active (non-deactivated) property IDs.
Includes both base ScriptComponent and ScriptButton-specific properties. Note that
min and max are deactivated on ScriptButton and will not appear in the result.

Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties()
