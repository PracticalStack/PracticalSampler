Node::setParent(var parentNode, Integer indexInParent) -> undefined

Thread safety: UNSAFE -- modifies ValueTree structure with undo manager. Removes from current parent and adds to new parent.
Moves this node to a new parent container at the specified index. Passing the DspNetwork
as parent auto-converts to the root node. Passing empty/null detaches the node. Existing
parameter connections are preserved via internal ScopedAutomationPreserver.
Dispatch/mechanics:
  removes from current parent -> network->get(parentNode) -> cast to NodeContainer
  -> adds to new parent's node tree at specified index
  ScopedAutomationPreserver preserves connections during move
Pair with:
  getChildNodes -- inspect the container's children after move
Anti-patterns:
  - [BUG] Node is removed from current parent before target validation. If the target
    is invalid, the node is already detached. Reversible via undo in backend builds.
Source:
  NodeBase.cpp  NodeBase::setParent()
    -> checkValid() -> ScopedAutomationPreserver
    -> v_data removed from current parent
    -> network->get(parentNode) -> NodeContainer::getNodeTree().addChild()
