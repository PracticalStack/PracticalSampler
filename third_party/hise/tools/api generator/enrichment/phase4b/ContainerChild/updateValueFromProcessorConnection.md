ContainerChild::updateValueFromProcessorConnection(Integer recursive) -> undefined

Thread safety: UNSAFE
Sends a message to refresh this component's value from its connected processor
parameter (set via processorId and parameterId properties). When recursive is
true, the message propagates to all descendant components.
Source:
  ScriptingApiContent.cpp  ChildReference::updateValueFromProcessorConnection()
    -> sendMessage(RefreshType::updateValueFromProcessorConnection, recursive)
    -> dispatched via refreshBroadcaster
