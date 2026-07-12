Connection::getTarget() -> ScriptObject

Thread safety: SAFE
Returns the target Parameter object of this connection. Resolved at construction
from NodeId and ParameterId properties in the connection ValueTree. Returns
undefined if the target parameter was not found or the target node was deleted.

Dispatch/mechanics:
  Returns cached WeakReference<NodeBase::Parameter> targetParameter
  Resolved once in constructor: network->getNodeWithId(nodeId) -> iterate parameters
    -> match by ParameterId

Pair with:
  getSourceNode -- get the other end of the connection
  isConnected -- verify connection validity

Source:
  NodeBase.cpp:1632  Constructor resolves targetParameter from NodeId + ParameterId properties
