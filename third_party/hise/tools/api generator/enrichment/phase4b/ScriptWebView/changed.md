ScriptWebView::changed() -> undefined

Thread safety: SAFE
Triggers the control callback (custom or default onControl) and notifies
registered value listeners.
Anti-patterns:
  - Do NOT call during onInit -- logs a console message and returns without
    executing.
  - If deferControlCallback is set, the callback is deferred to the message
    thread.
Source:
  ScriptingApiContent.cpp  ScriptComponent::changed() (base class, not overridden)
