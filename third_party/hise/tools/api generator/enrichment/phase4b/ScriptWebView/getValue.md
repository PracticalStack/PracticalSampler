ScriptWebView::getValue() -> var

Thread safety: SAFE
Returns the current value of the component. Uses SimpleReadWriteLock for
thread-safe read access.
Pair with:
  setValue -- sets the component value
  changed -- triggers control callback after value change
Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue() (base class)
