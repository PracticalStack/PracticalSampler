ScriptPanel::getAllProperties() -> Array

Thread safety: UNSAFE -- allocates Array
Returns an array of strings containing all active (non-deactivated) property IDs for
this component, including both base and ScriptPanel-specific properties.
Pair with:
  get -- read a property value by name
  set -- write a property value by name
Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties()
