ScriptedViewport::fadeComponent(Integer shouldBeVisible, Integer milliseconds) -> undefined

Thread safety: UNSAFE
Toggles visibility with a fade animation over the specified duration. Only triggers if target visibility differs from current. Sets the visible property and sends an async fade message through the global UI animator.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.fadeComponent(1, 500);
Dispatch/mechanics: Checks current visibility against target. If different, sets visible property and posts async fade message to the global ScriptComponentEditBroadcaster UI animator.
Pair with: showControl (instant visibility toggle without animation)
Source:
  ScriptingApiContent.cpp  ScriptComponent::fadeComponent() -> sendAsyncFadeMessage()
