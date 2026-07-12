ScriptImage::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Toggles visibility with a fade animation over the specified duration. Only triggers
if target visibility differs from current visibility. Sets the visible property and
sends an async fade message through the global UI animator.
Pair with:
  showControl -- instant visibility toggle (no animation)
  setAlpha -- controls static opacity; fadeComponent controls animated visibility
Anti-patterns:
  - Do NOT use milliseconds <= 0 -- animation requires a positive duration
Source:
  ScriptingApiContent.cpp  ScriptComponent::fadeComponent()
    -> sets visible property -> sends async fade message via UI animator
