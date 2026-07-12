Node::setBypassed(Integer shouldBeBypassed) -> undefined

Thread safety: SAFE -- sets a simple boolean member variable with no heap allocation or locking.
Sets the node's bypass state. Virtual method -- actual bypass behavior (silence output,
pass-through signal) depends on the node type's processing overrides. The ValueTree
Bypassed property is synchronized separately via a property listener.
Pair with:
  isBypassed -- reads the bypass state
  connectToBypass -- for dynamic bypass connections controlled by parameters
Source:
  NodeBase.h/NodeBase.cpp  NodeBase::setBypassed()
    -> sets bypassState member (virtual)
    -> ValueTree Bypassed property synchronized via bypassListener
