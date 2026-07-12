ScriptImage::sendRepaintMessage() -> undefined

Thread safety: UNSAFE
Sends an asynchronous repaint message via repaintBroadcaster. Use when visual
properties have been changed programmatically and a UI redraw is needed.
Source:
  ScriptingApiContent.cpp  ScriptComponent::sendRepaintMessage()
    -> repaintBroadcaster.sendMessage(sendNotificationAsync)
