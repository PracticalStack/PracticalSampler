Node::getNumParameters() -> Integer

Thread safety: SAFE
Returns the number of parameters on this node. Leaf nodes have a fixed count defined
by their factory type. Container nodes include dynamically created macro parameters.
Pair with:
  getOrCreateParameter -- retrieve or create parameters by index or name
Source:
  NodeBase.cpp  NodeBase::getNumParameters()
    -> returns parameters array size
