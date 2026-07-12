ScriptDynamicContainer::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Toggles visibility with a fade animation over the specified duration. Only triggers
if the target visibility differs from current state. Sets the visible property and
sends an async fade message through the global UI animator.
Pair with:
  showControl -- instant visibility toggle without animation
Source:
  ScriptingApiContent.cpp  ScriptComponent::fadeComponent()
    -> checks current visibility -> sets visible property -> sends async fade message
