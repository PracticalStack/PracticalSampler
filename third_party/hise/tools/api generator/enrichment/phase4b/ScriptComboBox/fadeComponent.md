ScriptComboBox::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Toggles visibility with a fade animation over the specified duration. Only
triggers if target visibility differs from current. Sets the visible property
and sends an async fade message through the global UI animator.
Pair with:
  showControl -- instant visibility toggle without animation
Source:
  ScriptingApiContent.h  ScriptComponent::fadeComponent()
