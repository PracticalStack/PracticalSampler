ScriptWebView::get(String propertyName) -> var

Thread safety: SAFE
Returns the current value of the named property. Reports a script error if the
property does not exist. See set() for the full property list.
Pair with:
  set -- sets a property value
Source:
  ScriptingApiContent.cpp  ScriptComponent::get() (base class)
