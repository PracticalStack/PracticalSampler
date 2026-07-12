ScriptWebView::setValue(NotUndefined newValue) -> undefined

Thread safety: SAFE
Sets the component's value. Thread-safe -- can be called from any thread; the
UI update happens asynchronously. Propagates to linked component targets and
sends value listener messages.
Pair with:
  getValue -- reads the current value
  changed -- triggers control callback after setting the value
Anti-patterns:
  - Do NOT pass a String value -- reports a script error
  - If called during onInit, the value will NOT be restored after recompilation
Source:
  ScriptingApiContent.cpp  ScriptComponent::setValue() (base class)
