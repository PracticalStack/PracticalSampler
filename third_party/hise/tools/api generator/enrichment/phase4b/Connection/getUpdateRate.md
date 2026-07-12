Connection::getUpdateRate() -> Integer

Thread safety: SAFE
Returns the block size of the lowest common container ancestor of the source and
target nodes. Reflects how often the modulation value updates -- smaller block sizes
mean higher update rates. Returns 0 if no common container was found.

Dispatch/mechanics:
  Returns commonContainer->getCurrentBlockRate()
  commonContainer resolved at construction: Helpers::findCommonParent(sourceInSignalChain, targetParameter)
    -> walks up ValueTree to lowest common ancestor

Pair with:
  getSourceNode -- identify which node produces the signal
  getTarget -- identify which parameter receives the value

Source:
  NodeBase.cpp  ConnectionBase::getUpdateRate()
  NodeBase.cpp:2112  Helpers::findCommonParent() -- recursive lowest-common-ancestor search
