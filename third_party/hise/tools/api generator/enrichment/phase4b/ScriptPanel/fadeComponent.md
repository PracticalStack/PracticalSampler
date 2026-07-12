ScriptPanel::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE -- sets visible property, sends async fade message
Toggles visibility with a fade animation over the specified duration. Only triggers
if the target visibility differs from the current state.
Dispatch/mechanics:
  Checks current visibility -> sets visible property -> sends async fade message
  through global UI animator
Pair with:
  showControl -- instant visibility toggle without animation
Source:
  ScriptingApiContent.cpp  ScriptComponent::fadeComponent()
