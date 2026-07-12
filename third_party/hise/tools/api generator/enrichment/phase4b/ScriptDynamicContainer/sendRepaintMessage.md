ScriptDynamicContainer::sendRepaintMessage() -> undefined

Thread safety: UNSAFE
Sends a repaint refresh through the dyncomp data model's refresh broadcaster,
triggering a visual update of all dynamic child components. Overrides the base
ScriptComponent implementation to propagate through the dynamic component tree
rather than just the container's own JUCE component.
Pair with:
  setData -- the data model that refresh propagates through
Source:
  ScriptingApiContent.cpp  ScriptDynamicContainer::sendRepaintMessage() [override]
    -> data->refreshBroadcaster.sendMessage(RefreshType::repaint)
