ScriptWebView::sendRepaintMessage() -> undefined

Thread safety: UNSAFE
Sends an asynchronous repaint message via repaintBroadcaster. ScriptWebView
manages its own rendering through the embedded browser engine, so this
primarily affects the component wrapper frame rather than the web content.
Source:
  ScriptingApiContent.cpp  ScriptComponent::sendRepaintMessage() (base class)
