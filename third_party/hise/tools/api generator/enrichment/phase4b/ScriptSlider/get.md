ScriptSlider::get(String propertyName) -> var

Thread safety: SAFE
Returns the current value of the named property. Reports a script error if the
property does not exist. See set() for the full property list.

Anti-patterns:
  - Do NOT use an invalid property name -- throws a script error

Source:
  ScriptingApiContent.cpp  ScriptComponent::get()
    -> property tree lookup, returns value or default
