ScriptWebView::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Toggles visibility with a fade animation over the specified duration. Only
triggers if the target visibility differs from the current state.
Pair with:
  showControl -- instant visibility toggle without animation
Source:
  ScriptingApiContent.cpp  ScriptComponent::fadeComponent() (base class)
