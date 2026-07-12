ContainerChild::loseFocus(Integer recursive) -> undefined

Thread safety: UNSAFE
Sends a message to the component to release keyboard focus. When recursive is
true, the message propagates to all descendant components.
Source:
  ScriptingApiContent.cpp  ChildReference::loseFocus()
    -> sendMessage(RefreshType::loseFocus, recursive)
    -> dispatched via refreshBroadcaster
