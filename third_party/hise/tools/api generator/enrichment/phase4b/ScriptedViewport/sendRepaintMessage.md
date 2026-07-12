ScriptedViewport::sendRepaintMessage() -> undefined

Thread safety: UNSAFE
Sends an asynchronous repaint message via repaintBroadcaster. Forces a UI redraw after programmatic visual property changes.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.sendRepaintMessage();
Source:
  ScriptingApiContent.cpp  ScriptComponent::sendRepaintMessage() -> repaintBroadcaster.sendMessage()
