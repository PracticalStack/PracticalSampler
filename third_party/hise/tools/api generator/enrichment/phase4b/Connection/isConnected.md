Connection::isConnected() -> Integer

Thread safety: SAFE
Returns whether this connection is still active. Checks if the underlying ValueTree
entry still has a valid parent. Returns false after disconnect() or if a connected
node was deleted.

Dispatch/mechanics:
  return data.getParent().isValid()
  Same check as objectDeleted()/objectExists() overrides

Pair with:
  disconnect -- the operation that invalidates a connection
  getSourceNode / getTarget -- guard these calls with isConnected() check

Source:
  NodeBase.h:713  objectDeleted() / objectExists() use same data.getParent().isValid() check
