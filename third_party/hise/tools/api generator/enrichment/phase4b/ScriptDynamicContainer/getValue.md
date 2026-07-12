ScriptDynamicContainer::getValue() -> var

Thread safety: SAFE
Returns the container's own ScriptComponent value. This is the container-level
value, not dyncomp child values. Use ContainerChild's getValue() or
setValueCallback for child component values.
Pair with:
  setValue -- set the container's own value
  changed -- trigger the control callback for this value
  setValueCallback -- listen to dyncomp child value changes (different system)
Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue()
