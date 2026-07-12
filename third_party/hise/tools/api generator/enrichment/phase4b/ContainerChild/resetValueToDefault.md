ContainerChild::resetValueToDefault(Integer recursive) -> undefined

Thread safety: UNSAFE
Sends a message to reset the component's value to the defaultValue property.
When recursive is true, the message propagates to all descendant components.
Source:
  ScriptingApiContent.cpp  ChildReference::resetValueToDefault()
    -> sendMessage(RefreshType::resetValueToDefault, recursive)
    -> dispatched via refreshBroadcaster
