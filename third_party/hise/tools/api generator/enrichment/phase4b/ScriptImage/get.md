ScriptImage::get(String propertyName) -> var

Thread safety: SAFE
Returns the current value of the named property. Reports a script error if the
property does not exist. See set() for the full property list.
Pair with:
  set -- sets the property value
  getAllProperties -- lists all valid property IDs
Anti-patterns:
  - Do NOT pass an invalid property name -- triggers a script error
Source:
  ScriptingApiContent.cpp  ScriptComponent::get()
    -> looks up property in ValueTree -> returns value or default
