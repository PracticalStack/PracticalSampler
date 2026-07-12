ScriptedViewport::getValue() -> var

Thread safety: SAFE
Returns the current component value. In List mode: selected row index (integer). In Table mode with MultiColumnMode: [column, row] array. In Viewport mode: whatever was last set via setValue(). Uses SimpleReadWriteLock for thread-safe read.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  var v = vp.getValue();
Pair with: setValue (sets the value), getValueNormalized (not applicable for viewport)
Anti-patterns: Stored value must not be a String -- assertion fires in debug builds if it is.
Source:
  ScriptingApiContent.cpp  ScriptComponent::getValue() -> SimpleReadWriteLock::ScopedReadLock -> value
