Node::isBypassed() -> Integer

Thread safety: SAFE
Returns whether the node is currently bypassed. Reads from the internal bypassState
member, not directly from the ValueTree property.
Pair with:
  setBypassed -- sets the bypass state
Source:
  NodeBase.h  NodeBase::isBypassed()
    -> returns bypassState member
