ScriptButton::setPropertiesFromJSON(JSON jsonData) -> undefined

Thread safety: UNSAFE
Sets multiple component properties at once from a JSON object. Each key must be a
valid property ID for this component type. Convenience method for batch assignment.

Pair with:
  set -- single property assignment
  get -- read back individual property values

Source:
  ScriptingApiContent.cpp  ScriptComponent::setPropertiesFromJSON()
