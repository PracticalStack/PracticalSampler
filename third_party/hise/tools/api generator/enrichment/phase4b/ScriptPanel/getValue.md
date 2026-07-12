ScriptPanel::getValue() -> var

Thread safety: SAFE
Returns the current value of the component. Uses SimpleReadWriteLock for
thread-safe read access.
Anti-patterns:
  - Do NOT store a String as the panel value -- assertion fires in debug builds
Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue()
