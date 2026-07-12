ScriptDynamicContainer::get(String propertyName) -> var

Thread safety: SAFE
Returns the current value of the named component property. Reports a script error
if the property does not exist. See set() for the full property list.
Pair with:
  set -- write a property value
  getAllProperties -- list available property names
Source:
  ScriptingApiContent.cpp  ScriptComponent::get()
    -> looks up property index -> returns from propertyTree
