Connection::disconnect() -> undefined

Thread safety: UNSAFE -- removes a ValueTree child with UndoManager, involving heap operations and listener notifications.
Removes this connection from the scriptnode graph. After calling, isConnected() returns false and getSourceNode() returns undefined.

Dispatch/mechanics:
  data.getParent().removeChild(data, undoManager)
    -> triggers ValueTree listener notifications
    -> source node rebuilds its DSP parameter chain via rebuildCallback()

Pair with:
  isConnected -- check validity after disconnect
  Node.connectTo / Parameter.addConnectionFrom -- to re-create a connection

Anti-patterns:
  - Do NOT continue using the Connection object after disconnect -- it is invalidated.
    getSourceNode() returns undefined, though getTarget() may still resolve via
    its independent WeakReference.

Source:
  NodeBase.cpp (ConnectionBase::disconnect)
    -> ValueTree::removeChild(data, undoManager)
    -> triggers ConnectionSourceManager::rebuildCallback()
